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
 * gtk3sermo-1.0.0 — Haplo-Linux (devel@haplo-dialog.fr), 2026
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
#include <glib.h>
#include "safe_exec.h"

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
 * _allowlist_permits:
 * HAPLO_ALLOWED_CMDS, quand elle est definie, restreint les commandes que le
 * programme accepte de lancer a une liste de noms separes par des virgules :
 *
 *     HAPLO_ALLOWED_CMDS=ls,cat,date
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
 * qu'il peut lancer. Meme famille que HAPLO_NO_SHELL_FALLBACK : une variable
 * d'environnement que pose l'exploitant, pas l'auteur du script.
 *
 * La comparaison porte sur le NOM DE BASE du programme, pas sur le chemin :
 * « /bin/ls » et « ls » sont le meme nom, sinon la liste se contournerait en
 * ecrivant le chemin absolu.
 *
 * Renvoie TRUE si la liste est absente (aucune restriction) ou si le nom y est.
 */
static gboolean _allowlist_permits(const gchar *argv0)
{
    const gchar  *list;
    gchar       **allowed;
    gchar        *base;
    gboolean      ok = FALSE;
    gint          i;

    list = g_getenv("HAPLO_ALLOWED_CMDS");
    if (list == NULL || *list == '\0')
        return TRUE;                 /* liste absente : aucune restriction */

    if (argv0 == NULL || *argv0 == '\0')
        return FALSE;

    base    = g_path_get_basename(argv0);
    allowed = g_strsplit(list, ",", -1);
    for (i = 0; allowed[i] != NULL; i++) {
        gchar *entry = g_strstrip(g_strdup(allowed[i]));
        if (*entry != '\0' && g_strcmp0(entry, base) == 0)
            ok = TRUE;
        g_free(entry);
        if (ok)
            break;
    }
    g_strfreev(allowed);

    if (!ok)
        g_critical("commande '%s' refusee : absente de HAPLO_ALLOWED_CMDS", base);
    g_free(base);
    return ok;
}

/*
 * _allowlist_is_active:
 * Vraie des que HAPLO_ALLOWED_CMDS est posee. Quand elle l'est, le repli
 * /bin/sh -c doit etre refuse : sinon « sh -c 'rm -rf /' » traverserait la
 * liste en s'appelant « sh », et la liste ne servirait a rien.
 */
static gboolean _allowlist_is_active(void)
{
    const gchar *list = g_getenv("HAPLO_ALLOWED_CMDS");
    return (list != NULL && *list != '\0');
}

static gchar **_build_child_env(void)
{
    gchar **src = g_get_environ();
    GPtrArray *out = g_ptr_array_new();
    guint i;

    for (i = 0; src && src[i]; i++) {
        const gchar *entry = src[i];
        const gchar *eq = strchr(entry, '=');
        gsize value_len;

        /* Drop the DIALOG description entirely — children re-derive it. */
        if (g_str_has_prefix(entry, "DIALOG="))
            continue;

        /* Drop any pathologically large variable defensively. */
        value_len = eq ? strlen(eq + 1) : 0;
        if (value_len > MAX_INHERITED_VALUE)
            continue;

        g_ptr_array_add(out, g_strdup(entry));
    }
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
        g_warning("safe_system: shell fallback (injection risk) for '%s'", command);
        if (g_getenv("HAPLO_NO_SHELL_FALLBACK")) {
            g_critical("safe_system: shell fallback refused (HAPLO_NO_SHELL_FALLBACK set)");
            return -1;
        }
        if (_allowlist_is_active()) {
            g_critical("safe_system: repli shell refuse tant que HAPLO_ALLOWED_CMDS est posee");
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
        g_warning("safe_popen: shell fallback (injection risk) for '%s'", command);
        if (g_getenv("HAPLO_NO_SHELL_FALLBACK")) {
            g_critical("safe_popen: shell fallback refused (HAPLO_NO_SHELL_FALLBACK set)");
            return NULL;
        }
        if (_allowlist_is_active()) {
            g_critical("safe_popen: repli shell refuse tant que HAPLO_ALLOWED_CMDS est posee");
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

    return stream;
}
