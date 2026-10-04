/*
 * safe_exec.c: Secure command execution wrappers.
 * Replaces direct calls to system() and popen() with GLib-based equivalents.
 *
 * Design notes:
 *
 * 1. METACHARACTERS: If the command string contains shell metacharacters
 *    (|, &, ;, <, >, (, ), {, }, $, `, \, ", ', ~, *) — as many gtk3sermo
 *    <input> commands do — g_shell_parse_argv() cannot tokenise them.
 *    In that case both safe_system() and safe_popen() fall back to
 *    /bin/sh -c, which preserves full shell functionality at the cost of
 *    re-enabling injection risk for that command.  The fall-back is logged
 *    via g_debug() so callers can audit it.
 *
 * 2. FCLOSE vs PCLOSE: safe_popen() returns FILE* via fdopen(), NOT via
 *    popen().  Callers MUST use fclose(), never pclose() — pclose() on an
 *    fdopen() FILE* is undefined behaviour.
 *
 * 3. ZOMBIE REAPING: G_SPAWN_DO_NOT_REAP_CHILD is used so we can attach
 *    a GChildWatch to reap the child automatically when the pipe is drained.
 *    This prevents zombie accumulation on timer/progressbar widgets.
 *
 * gtk3sermo-1.0.0 — haplo-dialog (devel@haplo-dialog.fr), 2026
 * License: GPL-2.0-or-later
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib.h>
#include "safe_exec.h"
#include "sermo_input.h"

/* Shell metacharacters that require /bin/sh -c fallback. */
#define SHELL_METACHARACTERS "|&;<>(){}$`\\\"'~*?"

/*
 * _has_shell_metacharacters:
 * Returns TRUE if the command contains any character that g_shell_parse_argv()
 * cannot handle and that requires a /bin/sh -c invocation.
 */
static gboolean _has_shell_metacharacters(const gchar *command)
{
    return strpbrk(command, SHELL_METACHARACTERS) != NULL;
}

/*
 * _build_child_env:
 * Build the environment passed to spawned children.  We start from the
 * current environment but DROP oversized variables that the child never
 * needs — most importantly DIALOG (the full XML description, several KiB),
 * which gtk3sermo itself reads only once at startup.  Inheriting it verbatim on
 * every <action> spawn bloats the child envp and, combined with a large
 * ambient environment, can push the total argv+envp payload past the
 * kernel's per-exec limit and yield a spurious E2BIG ("Argument list too
 * long") from g_spawn_*().  Any variable whose value exceeds
 * MAX_INHERITED_VALUE bytes is also dropped defensively.
 *
 * Caller frees the result with g_strfreev().
 */
#define MAX_INHERITED_VALUE 8192

/*
 * Variables de securite : nom actuel (SERMO_...) d'abord, puis le nom qu'elles
 * portaient en 1.x (HAPLO_...). Sans ce repli, un exploitant qui avait pose
 * HAPLO_ALLOWED_CMDS ou HAPLO_NO_SHELL_FALLBACK perdait sa protection en passant
 * a 2.x, sans un mot. L'ancien nom est encore lu, et signale une fois.
 */
static const gchar *_env_securite(const gchar *nom, const gchar *ancien,
                                  gboolean *deja_signale)
{
    const gchar *valeur = g_getenv(nom);

    if (valeur != NULL)
        return valeur;
    valeur = g_getenv(ancien);
    if (valeur != NULL && !*deja_signale) {
        *deja_signale = TRUE;
        g_warning("%s est l'ancien nom de %s : il est encore lu, renommez-le",
                  ancien, nom);
    }
    return valeur;
}

static const gchar *_allowed_cmds_env(void)
{
    static gboolean signale = FALSE;
    return _env_securite("SERMO_ALLOWED_CMDS", "HAPLO_ALLOWED_CMDS", &signale);
}

static gboolean _no_shell_fallback(void)
{
    static gboolean signale = FALSE;
    return _env_securite("SERMO_NO_SHELL_FALLBACK", "HAPLO_NO_SHELL_FALLBACK",
                         &signale) != NULL;
}

/*
 * _resoudre_dans_path:
 * Chemin auquel le PATH du programme resout un nom seul, comme le fera le
 * lancement (G_SPAWN_SEARCH_PATH), ou NULL. Ecrit ici plutot qu'appele a
 * g_find_program_in_path : la couche de compatibilite des backends sans GLib ne
 * le fournit pas.
 */
static gchar *_resoudre_dans_path(const gchar *nom)
{
    const gchar  *path = g_getenv("PATH");
    gchar       **dossiers;
    gchar        *trouve = NULL;
    gint          i;

    if (path == NULL || *path == '\0')
        path = "/bin:/usr/bin";
    dossiers = g_strsplit(path, ":", -1);
    for (i = 0; dossiers[i] != NULL && trouve == NULL; i++) {
        struct stat st;
        gchar *candidat = g_strdup_printf("%s/%s",
                                          *dossiers[i] ? dossiers[i] : ".", nom);
        if (stat(candidat, &st) == 0 && S_ISREG(st.st_mode)
            && access(candidat, X_OK) == 0)
            trouve = candidat;
        else
            g_free(candidat);
    }
    g_strfreev(dossiers);
    return trouve;
}

/*
 * _entree_autorise:
 * Une entree de la liste autorise-t-elle la commande telle qu'elle sera lancee ?
 *
 *  - meme ecriture : « ls » / « ls », ou le meme chemin des deux cotes ;
 *  - entree « ls », commande « /usr/bin/ls » : seulement si c'est exactement la
 *    que le PATH trouve ls ;
 *  - entree « /usr/local/bin/outil », commande « outil » : seulement si le PATH
 *    resout outil vers ce chemin exact.
 *
 * On compare des CHAINES, jamais des fichiers : suivre les liens symboliques d'un
 * chemin fourni par le script ouvrirait une course entre le controle et le
 * lancement.
 */
static gboolean _entree_autorise(const gchar *entree, const gchar *argv0)
{
    gboolean  ok = FALSE;
    gchar    *resolu;

    if (g_strcmp0(entree, argv0) == 0)
        return TRUE;
    if (strchr(entree, '/') == NULL && strchr(argv0, '/') != NULL) {
        resolu = _resoudre_dans_path(entree);
        ok = (resolu != NULL && g_strcmp0(resolu, argv0) == 0);
        g_free(resolu);
    } else if (strchr(entree, '/') != NULL && strchr(argv0, '/') == NULL) {
        resolu = _resoudre_dans_path(argv0);
        ok = (resolu != NULL && g_strcmp0(resolu, entree) == 0);
        g_free(resolu);
    }
    return ok;
}

/*
 * _allowlist_permits:
 * SERMO_ALLOWED_CMDS, quand elle est definie, restreint les commandes que le
 * programme accepte de lancer a une liste separee par des virgules :
 *
 *     SERMO_ALLOWED_CMDS=ls,cat,/usr/local/bin/outil
 *
 * ETEINTE PAR DEFAUT, et c'est un choix mesure. Le langage XML de sermo sert
 * precisement a lancer des commandes : les exemples livres en invoquent une
 * vingtaine par <input> et une soixantaine par <action>, et 14 d'entre eux
 * appellent bash ou sh directement. Une liste active par defaut casserait le
 * produit sans proteger personne — la commande vient du script que l'appelant
 * a ecrit lui-meme, et qui a deja un shell.
 *
 * Elle vise l'AUTRE cas, le seul reel : celui qui DEPLOIE un dialogue dans un
 * contexte moins fiable — une borne, une session invitee — et veut borner ce
 * qu'il peut lancer. Meme famille que SERMO_NO_SHELL_FALLBACK : une variable
 * d'environnement que pose l'exploitant, pas l'auteur du script.
 *
 * La comparaison porte sur la commande TELLE QU'ELLE SERA LANCEE (voir
 * _entree_autorise). Une version precedente ne comparait que le NOM DE BASE :
 * « /tmp/x/ls » passait des que « ls » etait liste, si bien que n'importe quel
 * executable nomme ls, pose n'importe ou, franchissait la liste.
 *
 * Renvoie TRUE si la liste est absente ou vide (aucune restriction) ou si une
 * entree autorise la commande.
 */
static gboolean _allowlist_permits(const gchar *argv0)
{
    const gchar  *list;
    gchar       **allowed;
    gboolean      ok = FALSE;
    gint          i;

    list = _allowed_cmds_env();
    if (list == NULL || *list == '\0')
        return TRUE;                 /* liste absente : aucune restriction */

    if (argv0 == NULL || *argv0 == '\0')
        return FALSE;

    allowed = g_strsplit(list, ",", -1);
    for (i = 0; allowed[i] != NULL && !ok; i++) {
        gchar *entry = g_strstrip(g_strdup(allowed[i]));
        if (*entry != '\0')
            ok = _entree_autorise(entry, argv0);
        g_free(entry);
    }
    g_strfreev(allowed);

    if (!ok)
        g_critical("commande '%s' refusee : absente de SERMO_ALLOWED_CMDS", argv0);
    return ok;
}

/*
 * _allowlist_is_active:
 * Vraie des que SERMO_ALLOWED_CMDS est posee. Quand elle l'est, le repli
 * /bin/sh -c doit etre refuse : sinon « sh -c 'rm -rf /' » traverserait la
 * liste en s'appelant « sh », et la liste ne servirait a rien.
 */
static gboolean _allowlist_is_active(void)
{
    const gchar *list = _allowed_cmds_env();
    return (list != NULL && *list != '\0');
}

static gchar **_build_child_env(void)
{
    gchar **src = g_get_environ();
    GPtrArray *out = g_ptr_array_new();
    gboolean allowlist = _allowlist_is_active();
    guint i;

    for (i = 0; src && src[i]; i++) {
        const gchar *entry = src[i];
        const gchar *eq = strchr(entry, '=');
        gsize value_len;

        /* Drop the DIALOG description entirely — children re-derive it. */
        if (g_str_has_prefix(entry, "DIALOG="))
            continue;

        /* Security fix (allowlist bypass): under the allowlist the dialog may be
         * hostile, yet a widget variable named LD_PRELOAD / LD_LIBRARY_PATH /
         * LD_AUDIT (any LD_*) is exported into our environment and would be
         * inherited here, loading attacker code into an otherwise-allowed command
         * and defeating the allowlist. Strip every LD_* variable, and drop PATH so
         * a sanitized one is pinned below (the bare-name allowlist check does not
         * resolve PATH, so a poisoned PATH could otherwise substitute the binary). */
        if (allowlist) {
            if (g_str_has_prefix(entry, "LD_"))
                continue;
            if (g_str_has_prefix(entry, "PATH="))
                continue;
        }

        /* Drop any pathologically large variable defensively. */
        value_len = eq ? strlen(eq + 1) : 0;
        if (value_len > MAX_INHERITED_VALUE)
            continue;

        g_ptr_array_add(out, g_strdup(entry));
    }
    /* Under the allowlist, pin a known-good PATH for the bare-name lookup. */
    if (allowlist)
        g_ptr_array_add(out, g_strdup("PATH=/usr/bin:/bin"));
    g_ptr_array_add(out, NULL);
    g_strfreev(src);

    return (gchar **)g_ptr_array_free(out, FALSE);
}

/*
 * _reap_child_cb:
 * GChildWatchFunc: called by the GLib main loop when a child process exits.
 * Closes the GPid handle to release the zombie.
 */
static void _reap_child_cb(GPid pid, gint status, gpointer user_data)
{
    (void)status;
    (void)user_data;
    g_spawn_close_pid(pid);
}

/*
 * safe_system:
 * Execute a shell command safely.  Uses g_spawn_sync() with direct exec()
 * when no metacharacters are present; falls back to /bin/sh -c otherwise.
 * Returns the exit status, or -1 on error.
 */
gint safe_system(const gchar *command)
{
    gchar  **argv        = NULL;
    gchar  **envp        = NULL;
    GError  *error       = NULL;
    gint     exit_status = -1;
    gboolean use_shell;

    if (!command || *command == '\0') {
        g_warning("safe_system: empty command");
        return -1;
    }
    if (strlen(command) > 65535) {
        g_warning("safe_system: command too long (%zu bytes)", strlen(command));
        return -1;
    }

    use_shell = _has_shell_metacharacters(command);

    if (use_shell) {
        /* Fait, pas alarme. La commande vient de l'auteur du script, qui a deja le
		 * droit de lancer des commandes : c'est le modele de confiance assume.
		 * Dire "injection risk" a chaque --do normal banalisait le message et le
		 * rendait inaudible le jour ou il compte. Le refus, lui, reste une alarme. */
		g_message("safe_system: '%s' contient de la syntaxe shell -- execution via /bin/sh -c", command);
        if (_no_shell_fallback()) {
            g_critical("safe_system: shell fallback refused (SERMO_NO_SHELL_FALLBACK set)");
            return -1;
        }
        if (_allowlist_is_active()) {
            g_critical("safe_system: repli shell refuse tant que SERMO_ALLOWED_CMDS est posee");
            return -1;
        }
        argv = g_new(gchar *, 4);
        argv[0] = g_strdup("/bin/sh");
        argv[1] = g_strdup("-c");
        argv[2] = g_strdup(command);
        argv[3] = NULL;
    } else {
        if (!g_shell_parse_argv(command, NULL, &argv, &error)) {
            g_warning("safe_system: cannot parse '%s': %s",
                      command, error->message);
            g_error_free(error);
            return -1;
        }
    }

    if (!_allowlist_permits(argv[0])) {
        g_strfreev(argv);
        return -1;
    }

    envp = _build_child_env();
    if (!g_spawn_sync(NULL, argv, envp,
                      G_SPAWN_SEARCH_PATH,
                      NULL, NULL,
                      NULL, NULL,
                      &exit_status,
                      &error)) {
        g_warning("safe_system: spawn failed for '%s': %s",
                  command, error->message);
        g_error_free(error);
        g_strfreev(envp);
        g_strfreev(argv);
        return -1;
    }

    g_strfreev(envp);
    g_strfreev(argv);
    return exit_status;
}

/*
 * safe_popen:
 * Open a read pipe to a command.  Returns FILE* (via fdopen) that the caller
 * closes with fclose() — NOT pclose().
 *
 * Uses g_spawn_async_with_pipes() with direct exec() when possible; falls
 * back to /bin/sh -c when the command contains shell metacharacters.
 * The child is reaped automatically via g_child_watch_add() to prevent
 * zombie accumulation.
 */
FILE *safe_popen(const gchar *command)
{
    gchar   **argv      = NULL;
    gchar   **envp      = NULL;
    GError   *error     = NULL;
    gint      stdout_fd = -1;
    GPid      child_pid;
    FILE     *stream;
    gboolean  use_shell;

    if (!command || *command == '\0') {
        g_warning("safe_popen: empty command");
        return NULL;
    }
    if (strlen(command) > 65535) {
        g_warning("safe_popen: command too long (%zu bytes)", strlen(command));
        return NULL;
    }

    use_shell = _has_shell_metacharacters(command);

    if (use_shell) {
        g_message("safe_popen: '%s' contient de la syntaxe shell -- execution via /bin/sh -c", command);
        if (_no_shell_fallback()) {
            g_critical("safe_popen: shell fallback refused (SERMO_NO_SHELL_FALLBACK set)");
            return NULL;
        }
        if (_allowlist_is_active()) {
            g_critical("safe_popen: repli shell refuse tant que SERMO_ALLOWED_CMDS est posee");
            return NULL;
        }
        argv = g_new(gchar *, 4);
        argv[0] = g_strdup("/bin/sh");
        argv[1] = g_strdup("-c");
        argv[2] = g_strdup(command);
        argv[3] = NULL;
    } else {
        if (!g_shell_parse_argv(command, NULL, &argv, &error)) {
            g_warning("safe_popen: cannot parse '%s': %s",
                      command, error->message);
            g_error_free(error);
            return NULL;
        }
    }

    if (!_allowlist_permits(argv[0])) {
        g_strfreev(argv);
        return NULL;
    }

    envp = _build_child_env();
    if (!g_spawn_async_with_pipes(
                NULL, argv, envp,
                G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                NULL, NULL,
                &child_pid,
                NULL,         /* stdin  — not needed */
                &stdout_fd,   /* stdout — we read from here */
                NULL,         /* stderr — inherit */
                &error)) {
        g_warning("safe_popen: spawn failed for '%s': %s",
                  command, error->message);
        g_error_free(error);
        g_strfreev(envp);
        g_strfreev(argv);
        return NULL;
    }

    g_strfreev(envp);
    g_strfreev(argv);

    /* Register a child watcher so the GLib main loop reaps the zombie
     * automatically once the child exits, without any additional caller
     * action required. */
    g_child_watch_add(child_pid, _reap_child_cb, NULL);

    /* Wrap the raw fd in a FILE* — caller uses fgets()/fclose(). */
    stream = fdopen(stdout_fd, "r");
    if (!stream) {
        g_warning("safe_popen: fdopen failed for '%s'", command);
        close(stdout_fd);
        /* Child watcher will still reap the child correctly. */
        return NULL;
    }

    /* 2.7.3 : ce que lit un <input> est plafonné (SERMO_INPUT_MAX, 16 Mio par
     * défaut). Une commande sans fin (`yes`) épuisait la mémoire ; coupée, elle
     * reçoit SIGPIPE à la fermeture du flux. Contrat : sermo_input.h. */
    return sermo_input_wrap(stream, command);
}
