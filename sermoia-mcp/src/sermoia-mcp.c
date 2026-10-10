/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermoia-mcp.c — serveur MCP pour sermo : la boucle IA
 *   apprendre → générer → vérifier → (voir) → corriger.
 *
 * Réécriture CLEAN-ROOM des CONCEPTS (schéma / check / render / run) — aucun
 * code repris d'ailleurs. Enveloppe les binaires sermo existants :
 *   - sermo_schema : décrit le langage XML de sermo en JSON (l'IA l'apprend) ;
 *   - sermo_check  : valide un XML via `sermo --file=… --print-ir` ;
 *   - sermo_run    : affiche le dialogue et renvoie les valeurs (VAR="…") ;
 *   - sermo_render : dessine en PNG sans afficher (à venir : primitive
 *                    `--render-png` côté backend / capture xvfb).
 *
 * Protocole : JSON-RPC 2.0 sur stdio, messages délimités par newline (spec MCP).
 * Binaire sermo utilisé : $SERMO_BIN (défaut « sermo »).
 *
 * Copyright (C) 2026 haplo-dialog <devel@haplo-dialog.fr>. GPL-2.0-or-later.
 */
#include <glib.h>
#include <glib/gstdio.h>
#include <json-glib/json-glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Version : posée par CMake depuis le fichier VERSION du dépôt (une seule
 * source, voir VERSIONING.md) ; la valeur ci-dessous ne sert qu'hors CMake. */
#ifndef SERMOIA_MCP_VERSION
#define SERMOIA_MCP_VERSION "inconnue"
#endif

static const char *sermo_bin(void)
{
    const char *b = g_getenv("SERMO_BIN");
    return (b && *b) ? b : "sermo";
}

/* ─── Schéma du langage (clean-room, depuis la doc/tables de sermo) ─────────── */
static const char *SCHEMA_JSON =
"{"
"\"langage\":\"sermo-xml\",\"racine\":\"window\","
"\"conteneurs\":[\"window\",\"vbox\",\"hbox\",\"frame\",\"notebook\",\"expander\",\"eventbox\",\"aspectframe\",\"grid\",\"paned\",\"stack\",\"wizard\",\"toolbar\",\"flowbox\",\"overlay\",\"revealer\"],"
"\"affichage\":[\"text\",\"image\",\"pixmap\",\"statusbar\",\"infobar\",\"drawingarea\",\"progressbar\",\"pulse\",\"spinner\",\"levelbar\"],"
"\"saisie\":[\"entry\",\"edit\",\"password\",\"searchentry\",\"spinbutton\",\"hscale\",\"vscale\",\"checkbox\",\"togglebutton\",\"radiobutton\",\"switch\"],"
"\"selection\":[\"combobox\",\"comboboxtext\",\"comboboxentry\",\"chooser\",\"list\",\"tree\",\"table\",\"colorbutton\",\"fontbutton\",\"filechooser\",\"calendar\"],"
"\"boutons\":[\"button\",\"linkbutton\",\"menubutton\"],\"stock\":[\"button ok\",\"button cancel\",\"button help\",\"button yes\",\"button no\"],"
"\"menus\":[\"menubar\",\"menu\",\"menuitem\",\"menuitemseparator\"],"
"\"divers\":[\"terminal\",\"hseparator\",\"vseparator\",\"timer\"],"
"\"balises_contenu\":[\"variable\",\"label\",\"default\",\"input\",\"action\",\"item\",\"sensitive\",\"visible\"],"
"\"attributs_communs\":[\"sensitive\",\"visible\",\"tooltip-text\",\"width-request\",\"height-request\",\"space-expand\",\"space-fill\"],"
"\"actions\":[\"EXIT\",\"CLOSEWINDOW\",\"LAUNCH\",\"REFRESH\",\"SAVE\",\"CLEAR\",\"APPEND\",\"INSERT\",\"FILESELECT\",\"ENABLE\",\"DISABLE\",\"SHOW\",\"HIDE\",\"GRABFOCUS\",\"PRESENTWINDOW\",\"PULSE\",\"START\",\"STOP\",\"ACTIVATE\",\"REMOVESELECTED\",\"BREAK\"],"
"\"signaux\":[\"activate\",\"changed\",\"clicked\",\"toggled\",\"value-changed\",\"cursor-changed\",\"select-row\"],"
"\"sortie\":\"À la fermeture, chaque widget nommé émet NOM=\\\"valeur\\\" sur stdout (échappé pour eval). Préférer --do : les valeurs arrivent par l'environnement, jamais relues comme du code.\","
"\"regle\":\"Racine <window>. Widgets imbriqués dans des conteneurs. <variable> nomme la sortie. <action>ACTION:cible</action> ou commande shell.\""
"}";

/* ─── Exécution d'un binaire sermo sur un XML (via fichier temporaire) ──────── */
/* Écrit `xml` dans un fichier temporaire, lance `sermo <extra_args> --file=tmp`,
 * capture stdout/stderr/exit. Renvoie TRUE si le spawn a réussi (≠ exit code). */
static gboolean run_sermo_on_xml(const char *xml, const char *const *extra,
                                 gchar **out, gchar **err, gint *status)
{
    gchar *tmpl = g_build_filename(g_get_tmp_dir(), "sermoia-XXXXXX.xml", NULL);
    GError *e = NULL;
    gint fd = g_mkstemp(tmpl);
    if (fd < 0) { g_free(tmpl); return FALSE; }
    if (xml) { if (write(fd, xml, strlen(xml)) < 0) { /* best-effort */ } }
    close(fd);

    GPtrArray *argv = g_ptr_array_new();
    g_ptr_array_add(argv, (gpointer)sermo_bin());
    for (int i = 0; extra && extra[i]; i++) g_ptr_array_add(argv, (gpointer)extra[i]);
    gchar *file_arg = g_strdup_printf("--file=%s", tmpl);
    g_ptr_array_add(argv, file_arg);
    g_ptr_array_add(argv, NULL);

    gboolean ok = g_spawn_sync(NULL, (gchar **)argv->pdata, NULL,
                               G_SPAWN_SEARCH_PATH, NULL, NULL,
                               out, err, status, &e);
    if (!ok && e) { if (err) { g_free(*err); *err = g_strdup(e->message); } g_error_free(e); }
    g_ptr_array_free(argv, TRUE);
    g_free(file_arg);
    g_unlink(tmpl);
    g_free(tmpl);
    return ok;
}

/* ─── Fabrique d'un résultat tools/call : {content:[{type:text,text:…}]} ────── */
static JsonObject *tool_text_result(const char *text, gboolean is_error)
{
    JsonObject *res = json_object_new();
    JsonArray *content = json_array_new();
    JsonObject *item = json_object_new();
    json_object_set_string_member(item, "type", "text");
    json_object_set_string_member(item, "text", text ? text : "");
    json_array_add_object_element(content, item);
    json_object_set_array_member(res, "content", content);
    if (is_error) json_object_set_boolean_member(res, "isError", TRUE);
    return res;
}

/* ─── Résultat image : {content:[{type:image,data:base64,mimeType:…}]} ─────── */
static JsonObject *tool_image_result(const char *b64, const char *mime)
{
    JsonObject *res = json_object_new();
    JsonArray *content = json_array_new();
    JsonObject *item = json_object_new();
    json_object_set_string_member(item, "type", "image");
    json_object_set_string_member(item, "data", b64 ? b64 : "");
    json_object_set_string_member(item, "mimeType", mime ? mime : "image/png");
    json_array_add_object_element(content, item);
    json_object_set_array_member(res, "content", content);
    return res;
}

/* ─── Rendu en PNG, déterministe via --render-png. Deux voies : plateformes
 * offscreen posées (qt6/efl1/sdl3), sinon --render-png sous xvfb où le backend
 * écrit lui-même le PNG (gtk3/gtk4/fltk1). Les 6 backends GUI le gèrent ; un
 * backend terminal (ncurses) retombe en aperçu texte (--print-ir). SERMO_RENDER_BIN désigne le binaire GUI (défaut
 * : SERMO_BIN). Renvoie le PNG en base64, ou NULL. Cf. feuille de route. */
static gboolean png_ok(const char *path)
{
    gchar *data = NULL; gsize len = 0; gboolean ok = FALSE;
    if (g_file_get_contents(path, &data, &len, NULL) && len > 8 &&
        (guchar)data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
        ok = TRUE;
    g_free(data);
    return ok;
}

/* Voie propre HEADLESS : `rbin --render-png OUT --file=XML` avec les plateformes
 * offscreen de chaque toolkit posées (Qt: offscreen ; SDL3: offscreen ; EFL:
 * buffer). Déterministe, sans display ni xvfb ni capture externe. Marche pour
 * qt6 / efl1 / sdl3. */
static gboolean render_via_primitive(const char *rbin, const char *xmlf, const char *pngf)
{
    gchar *fileArg = g_strdup_printf("--file=%s", xmlf);
    const gchar *argv[] = { rbin, "--render-png", pngf, fileArg, NULL };
    gchar **env = g_get_environ();
    env = g_environ_setenv(env, "QT_QPA_PLATFORM", "offscreen", TRUE);  /* qt6 */
    env = g_environ_setenv(env, "SDL_VIDEODRIVER", "offscreen", TRUE);  /* sdl3 */
    env = g_environ_setenv(env, "ELM_ENGINE", "buffer", TRUE);          /* efl1 */
    env = g_environ_setenv(env, "SERMO_HTML_OUT", "/dev/null", TRUE);   /* html : pas de page écrite */
    gint st = 0;
    /* Sorties du backend vers /dev/null : la sortie standard de ce processus est
     * le canal MCP. Un backend qui ignore --render-png (html, ncurses) y
     * écrirait ses lignes VAR="…" et casserait le protocole (mesuré). */
    gboolean ok = g_spawn_sync(NULL, (gchar **)argv, env,
                               G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                               NULL, NULL, NULL, NULL, &st, NULL);
    g_strfreev(env); g_free(fileArg);
    return ok && png_ok(pngf);
}

/* Repli DÉTERMINISTE : `xvfb-run -a rbin --render-png OUT --file=XML`. Pour les
 * toolkits qui exigent un display même offscreen (gtk3/gtk4/fltk1) : le backend
 * écrit lui-même le PNG (pas de capture `import`), on ne fournit que le display. */
static gboolean render_via_xvfb(const char *rbin, const char *xmlf, const char *pngf)
{
    gchar *fileArg = g_strdup_printf("--file=%s", xmlf);
    const gchar *argv[] = { "xvfb-run", "-a", rbin, "--render-png", pngf, fileArg, NULL };
    gchar **env = g_environ_setenv(g_get_environ(), "SERMO_HTML_OUT", "/dev/null", TRUE);
    gint st = 0;
    /* Même règle que ci-dessus : rien du backend ne doit atteindre le canal MCP. */
    gboolean ok = g_spawn_sync(NULL, (gchar **)argv, env,
                               G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                               NULL, NULL, NULL, NULL, &st, NULL);
    g_strfreev(env); g_free(fileArg);
    return ok && png_ok(pngf);
}

static gchar *render_png_b64(const char *xml)
{
    const char *rbin = g_getenv("SERMO_RENDER_BIN");
    if (!rbin || !*rbin) rbin = sermo_bin();

    gchar *xmlf = g_build_filename(g_get_tmp_dir(), "sermoia-r-XXXXXX.xml", NULL);
    gint fd = g_mkstemp(xmlf);
    if (fd < 0) { g_free(xmlf); return NULL; }
    if (xml) { if (write(fd, xml, strlen(xml)) < 0) {} }
    close(fd);
    gchar *pngf = g_build_filename(g_get_tmp_dir(), "sermoia-r-XXXXXX.png", NULL);
    gint pfd = g_mkstemp(pngf);
    if (pfd >= 0) close(pfd);

    /* Voie propre d'abord (--render-png), repli xvfb si non supporté. */
    gboolean ok = render_via_primitive(rbin, xmlf, pngf)
               || render_via_xvfb(rbin, xmlf, pngf);

    gchar *b64 = NULL;
    if (ok) {
        gchar *data = NULL; gsize len = 0;
        if (g_file_get_contents(pngf, &data, &len, NULL) && len > 0)
            b64 = g_base64_encode((const guchar *)data, len);
        g_free(data);
    }
    g_unlink(xmlf); g_unlink(pngf);
    g_free(xmlf); g_free(pngf);
    return b64;
}

/* ─── RAG/CAG : récupération d'exemples validés par sujet (cookbook) ────────
 * Parcourt SERMO_EXAMPLES_DIR (sous-dossiers = sujets : button, calendar,
 * filechooser, system-tools…) et renvoie les scripts dont le nom de sujet
 * contient le mot-clé `task`. Modulaire : c'est le socle CAG « exemples validés
 * en contexte », trié par le vote append-only ci-dessous.
 * Recherche 100% en C (pas de shell) → pas d'injection via `task`. */
static const char *examples_dir(void)
{
    const char *d = g_getenv("SERMO_EXAMPLES_DIR");
    return (d && *d) ? d : "examples";
}
static gboolean ci_contains(const char *hay, const char *needle)
{
    if (!needle || !*needle) return TRUE;   /* pas de filtre → tout */
    gchar *h = g_utf8_casefold(hay, -1), *n = g_utf8_casefold(needle, -1);
    gboolean r = (strstr(h, n) != NULL);
    g_free(h); g_free(n);
    return r;
}
/* ─── Vote de priorité (« fiche qui trompe / qui sauve »), APPEND-ONLY ───────
 * Fichier TSV : clé<TAB>delta(+1/-1)<TAB>note. On n'efface JAMAIS un vote (comme
 * le registre RAG Haplo). SERMO_VOTE_FILE, défaut : état utilisateur. */
static gchar *vote_file(void)
{
    const char *v = g_getenv("SERMO_VOTE_FILE");
    if (v && *v) return g_strdup(v);
    return g_build_filename(g_get_user_state_dir(), "sermoia", "votes.tsv", NULL);
}
static void append_vote(const char *key, int delta, const char *note)
{
    gchar *vf = vote_file(), *dir = g_path_get_dirname(vf);
    g_mkdir_with_parents(dir, 0700);
    FILE *fp = fopen(vf, "a");
    if (fp) {
        gchar *n1 = g_strdelimit(g_strdup(note ? note : ""), "\t\n", ' ');
        fprintf(fp, "%s\t%d\t%s\n", key, delta, n1);
        fclose(fp); g_free(n1);
    }
    g_free(dir); g_free(vf);
}
static int subject_score(const char *key)
{
    gchar *vf = vote_file(), *data = NULL; gsize len = 0; int score = 0;
    if (g_file_get_contents(vf, &data, &len, NULL)) {
        gchar **lines = g_strsplit(data, "\n", -1);
        for (int i = 0; lines[i]; i++) {
            gchar **f = g_strsplit(lines[i], "\t", 3);
            if (f[0] && f[1] && g_strcmp0(f[0], key) == 0) score += atoi(f[1]);
            g_strfreev(f);
        }
        g_strfreev(lines);
    }
    g_free(data); g_free(vf);
    return score;
}

typedef struct { gchar *name; int score; } Subj;
static int subj_cmp(const void *a, const void *b)
{ return ((const Subj *)b)->score - ((const Subj *)a)->score; }

static JsonObject *tool_examples(const char *task)
{
    GString *out = g_string_new(NULL);

    /* Catalogue local, TRIÉ PAR VOTE (priorité aux exemples qui ont sauvé). */
    const char *root = examples_dir();
    GDir *d = g_dir_open(root, 0, NULL);
    if (d) {
        GArray *subs = g_array_new(FALSE, FALSE, sizeof(Subj));
        const char *sub;
        while ((sub = g_dir_read_name(d))) {
            gchar *subpath = g_build_filename(root, sub, NULL);
            if (g_file_test(subpath, G_FILE_TEST_IS_DIR) && ci_contains(sub, task)) {
                Subj s = { g_strdup(sub), subject_score(sub) };
                g_array_append_val(subs, s);
            }
            g_free(subpath);
        }
        g_dir_close(d);
        qsort(subs->data, subs->len, sizeof(Subj), subj_cmp);

        int shown = 0;
        for (guint i = 0; i < subs->len && shown < 6; i++) {
            Subj *s = &g_array_index(subs, Subj, i);
            gchar *subpath = g_build_filename(root, s->name, NULL);
            GDir *sd = g_dir_open(subpath, 0, NULL);
            const char *f;
            while (sd && (f = g_dir_read_name(sd)) && shown < 6) {
                gchar *fp = g_build_filename(subpath, f, NULL);
                gchar *content = NULL; gsize len = 0;
                if (g_file_test(fp, G_FILE_TEST_IS_REGULAR) &&
                    g_file_get_contents(fp, &content, &len, NULL) && len > 0) {
                    g_string_append_printf(out, "===== %s/%s  [vote:%d] =====\n", s->name, f, s->score);
                    g_string_append_len(out, content, len > 1400 ? 1400 : (gssize)len);
                    if (len > 1400) g_string_append(out, "\n… (tronqué)\n");
                    g_string_append_c(out, '\n');
                    shown++;
                }
                g_free(content); g_free(fp);
            }
            if (sd) g_dir_close(sd);
            g_free(subpath);
        }
        for (guint i = 0; i < subs->len; i++) g_free(g_array_index(subs, Subj, i).name);
        g_array_free(subs, TRUE);
    } else {
        gchar *m = g_strdup_printf("Catalogue introuvable (SERMO_EXAMPLES_DIR=%s).", root);
        JsonObject *r = tool_text_result(m, TRUE);
        g_free(m); g_string_free(out, TRUE); return r;
    }

    if (out->len == 0)
        g_string_append_printf(out, "Aucun exemple pour « %s ». Lancer sans filtre pour la liste des sujets.", task ? task : "");
    JsonObject *r = tool_text_result(out->str, FALSE);
    g_string_free(out, TRUE);
    return r;
}

/* ─── sermo_vote : enregistre qu'un exemple a aidé (+1) ou trompé (-1) ──────── */
static JsonObject *tool_vote(const char *example, gboolean helpful, const char *note)
{
    if (!example || !*example) return tool_text_result("Argument 'example' manquant.", TRUE);
    gchar **parts = g_strsplit(example, "/", 2);
    const char *key = (parts[0] && *parts[0]) ? parts[0] : example;  /* sujet */
    append_vote(key, helpful ? +1 : -1, note);
    int sc = subject_score(key);
    gchar *m = g_strdup_printf("Vote enregistré (append-only) : %s %s → score cumulé %d. "
        "sermo_examples priorisera les sujets au score le plus haut.",
        key, helpful ? "a aidé (+1)" : "a trompé (-1)", sc);
    g_strfreev(parts);
    JsonObject *r = tool_text_result(m, FALSE); g_free(m); return r;
}

/* ─── tools/list ───────────────────────────────────────────────────────────── */
static void add_tool(JsonArray *arr, const char *name, const char *desc, gboolean needs_xml)
{
    JsonObject *t = json_object_new();
    json_object_set_string_member(t, "name", name);
    json_object_set_string_member(t, "description", desc);
    JsonObject *schema = json_object_new();
    json_object_set_string_member(schema, "type", "object");
    JsonObject *props = json_object_new();
    if (needs_xml) {
        JsonObject *xmlp = json_object_new();
        json_object_set_string_member(xmlp, "type", "string");
        json_object_set_string_member(xmlp, "description", "Le script XML sermo (racine <window>).");
        json_object_set_object_member(props, "xml", xmlp);
    }
    json_object_set_object_member(schema, "properties", props);
    if (needs_xml) {
        JsonArray *req = json_array_new();
        json_array_add_string_element(req, "xml");
        json_object_set_array_member(schema, "required", req);
    }
    json_object_set_object_member(t, "inputSchema", schema);
    json_array_add_object_element(arr, t);
}

static JsonArray *tools_list(void)
{
    JsonArray *a = json_array_new();
    add_tool(a, "sermo_schema", "Renvoie la grammaire du langage sermo (JSON) : conteneurs, widgets, attributs, actions, signaux. L'IA l'apprend une fois.", FALSE);
    add_tool(a, "sermo_check",  "Valide un XML sermo (parse via --print-ir). Renvoie {ok:…} ; en cas d'erreur, le message pour se corriger.", TRUE);
    add_tool(a, "sermo_run",    "Affiche le dialogue à l'utilisateur et renvoie les valeurs saisies (lignes VAR=\"…\").", TRUE);
    add_tool(a, "sermo_render", "Dessine la fenêtre en PNG (image) via un backend GUI headless (xvfb) — pour que l'IA « voie » le dialogue.", TRUE);
    /* sermo_examples : RAG/CAG sur le catalogue d'exemples validés (param 'task'). */
    {
        JsonObject *t = json_object_new();
        json_object_set_string_member(t, "name", "sermo_examples");
        json_object_set_string_member(t, "description",
            "RAG/CAG : renvoie des exemples XML VALIDÉS par sujet (button, calendar, filechooser, system-tools…). "
            "Donner 'task' (mot-clé) pour filtrer ; vide = survol. Trié par le vote (sermo_vote).");
        JsonObject *schema = json_object_new();
        json_object_set_string_member(schema, "type", "object");
        JsonObject *props = json_object_new();
        JsonObject *taskp = json_object_new();
        json_object_set_string_member(taskp, "type", "string");
        json_object_set_string_member(taskp, "description", "Mot-clé de sujet (optionnel).");
        json_object_set_object_member(props, "task", taskp);
        json_object_set_object_member(schema, "properties", props);
        json_object_set_object_member(t, "inputSchema", schema);
        json_array_add_object_element(a, t);
    }
    /* sermo_vote : retour d'expérience sur un exemple (priorité append-only). */
    {
        JsonObject *t = json_object_new();
        json_object_set_string_member(t, "name", "sermo_vote");
        json_object_set_string_member(t, "description",
            "Vote (append-only) : signale qu'un exemple a AIDÉ (helpful=true) ou TROMPÉ (helpful=false). "
            "sermo_examples priorise ensuite les sujets au score le plus haut.");
        JsonObject *schema = json_object_new();
        json_object_set_string_member(schema, "type", "object");
        JsonObject *props = json_object_new();
        JsonObject *ex = json_object_new();
        json_object_set_string_member(ex, "type", "string");
        json_object_set_string_member(ex, "description", "Sujet ou 'sujet/fichier' de l'exemple voté.");
        json_object_set_object_member(props, "example", ex);
        JsonObject *hp = json_object_new();
        json_object_set_string_member(hp, "type", "boolean");
        json_object_set_string_member(hp, "description", "true = a aidé, false = a trompé.");
        json_object_set_object_member(props, "helpful", hp);
        JsonObject *nt = json_object_new();
        json_object_set_string_member(nt, "type", "string");
        json_object_set_string_member(nt, "description", "Note libre (optionnel).");
        json_object_set_object_member(props, "note", nt);
        json_object_set_object_member(schema, "properties", props);
        JsonArray *req = json_array_new();
        json_array_add_string_element(req, "example");
        json_array_add_string_element(req, "helpful");
        json_object_set_array_member(schema, "required", req);
        json_object_set_object_member(t, "inputSchema", schema);
        json_array_add_object_element(a, t);
    }
    return a;
}

/* ─── tools/call ───────────────────────────────────────────────────────────── */
static JsonObject *tool_call(const char *name, JsonObject *args)
{
    const char *xml = (args && json_object_has_member(args, "xml"))
                    ? json_object_get_string_member(args, "xml") : NULL;

    if (g_strcmp0(name, "sermo_schema") == 0)
        return tool_text_result(SCHEMA_JSON, FALSE);

    if (g_strcmp0(name, "sermo_examples") == 0) {
        const char *task = (args && json_object_has_member(args, "task"))
                         ? json_object_get_string_member(args, "task") : NULL;
        return tool_examples(task);
    }

    if (g_strcmp0(name, "sermo_vote") == 0) {
        const char *ex = (args && json_object_has_member(args, "example"))
                       ? json_object_get_string_member(args, "example") : NULL;
        gboolean helpful = args && json_object_has_member(args, "helpful")
                         ? json_object_get_boolean_member(args, "helpful") : TRUE;
        const char *note = (args && json_object_has_member(args, "note"))
                         ? json_object_get_string_member(args, "note") : NULL;
        return tool_vote(ex, helpful, note);
    }

    if (g_strcmp0(name, "sermo_check") == 0) {
        if (!xml) return tool_text_result("Argument 'xml' manquant.", TRUE);
        gchar *out = NULL, *err = NULL; gint st = 0;
        const char *extra[] = { "--print-ir", NULL };
        gboolean ok = run_sermo_on_xml(xml, extra, &out, &err, &st);
        gboolean valid = ok && g_spawn_check_wait_status(st, NULL);
        gchar *msg = g_strdup_printf("{\"ok\":%s,%s\"sortie\":%s}",
            valid ? "true" : "false",
            valid ? "" : "\"probleme\":true,",
            "\"(voir texte)\"");
        gchar *full = g_strdup_printf("%s\n--- IR/sortie ---\n%s%s%s",
            msg, out ? out : "", (err && *err) ? "\n--- diagnostic ---\n" : "", err ? err : "");
        JsonObject *r = tool_text_result(full, !valid);
        g_free(msg); g_free(full); g_free(out); g_free(err);
        return r;
    }

    if (g_strcmp0(name, "sermo_run") == 0) {
        if (!xml) return tool_text_result("Argument 'xml' manquant.", TRUE);
        gchar *out = NULL, *err = NULL; gint st = 0;
        const char *extra[] = { NULL };   /* --file=tmp : affiche + exporte */
        gboolean ok = run_sermo_on_xml(xml, extra, &out, &err, &st);
        if (!ok) { JsonObject *r = tool_text_result(err ? err : "échec du lancement de sermo", TRUE);
                   g_free(out); g_free(err); return r; }
        JsonObject *r = tool_text_result(out && *out ? out : "(aucune valeur exportée)", FALSE);
        g_free(out); g_free(err);
        return r;
    }

    if (g_strcmp0(name, "sermo_render") == 0) {
        if (!xml) return tool_text_result("Argument 'xml' manquant.", TRUE);
        /* Fonctionne avec TOUS les ports : image PNG pour un backend GUI ; si
         * le rendu est vide/absent (backend terminal comme ncurses, ou capture
         * ratée) → repli en APERÇU STRUCTUREL texte (--print-ir), toujours utile. */
        gchar *b64 = render_png_b64(xml);
        gsize approx = b64 ? (strlen(b64) / 4) * 3 : 0;   /* octets ≈ b64*3/4 */
        if (b64 && approx >= 800) {                        /* PNG non vide */
            JsonObject *r = tool_image_result(b64, "image/png");
            g_free(b64);
            return r;
        }
        g_free(b64);
        /* repli texte : représentation structurelle du dialogue */
        gchar *out = NULL, *err = NULL; gint st = 0;
        const char *extra[] = { "--print-ir", NULL };
        run_sermo_on_xml(xml, extra, &out, &err, &st);
        gchar *msg = g_strdup_printf(
            "Aperçu STRUCTUREL (ce backend ne produit pas d'image exploitable — "
            "backend terminal comme ncurses, ou rendu image vide) :\n"
            "--- représentation interne (widgets/attributs) ---\n%s", out ? out : "(indisponible)");
        JsonObject *r = tool_text_result(msg, FALSE);
        g_free(msg); g_free(out); g_free(err);
        return r;
    }

    gchar *m = g_strdup_printf("Outil inconnu : %s", name ? name : "(nul)");
    JsonObject *r = tool_text_result(m, TRUE);
    g_free(m);
    return r;
}

/* ─── Émission d'une réponse JSON-RPC (une ligne) ──────────────────────────── */
static void send_response(JsonNode *id, JsonObject *result, const char *err_msg)
{
    JsonObject *root = json_object_new();
    json_object_set_string_member(root, "jsonrpc", "2.0");
    if (id) json_object_set_member(root, "id", json_node_copy(id));
    else    json_object_set_null_member(root, "id");
    if (err_msg) {
        JsonObject *e = json_object_new();
        json_object_set_int_member(e, "code", -32603);
        json_object_set_string_member(e, "message", err_msg);
        json_object_set_object_member(root, "error", e);
        if (result) json_object_unref(result);
    } else {
        json_object_set_object_member(root, "result", result ? result : json_object_new());
    }
    JsonNode *rn = json_node_new(JSON_NODE_OBJECT);
    json_node_take_object(rn, root);
    JsonGenerator *g = json_generator_new();
    json_generator_set_root(g, rn);
    gchar *line = json_generator_to_data(g, NULL);
    printf("%s\n", line);
    fflush(stdout);
    g_free(line);
    g_object_unref(g);
    json_node_free(rn);
}

/* ─── Boucle principale : JSON-RPC délimité par newline sur stdin ───────────── */
int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    GString *line = g_string_new(NULL);
    int c;
    while ((c = getc(stdin)) != EOF) {
        if (c != '\n') { g_string_append_c(line, (gchar)c); continue; }
        if (line->len == 0) continue;

        JsonParser *p = json_parser_new();
        gboolean parsed = json_parser_load_from_data(p, line->str, line->len, NULL);
        g_string_set_size(line, 0);
        if (!parsed) { g_object_unref(p); continue; }

        JsonObject *req = json_node_get_object(json_parser_get_root(p));
        const char *method = json_object_has_member(req, "method")
                           ? json_object_get_string_member(req, "method") : "";
        JsonNode *id = json_object_has_member(req, "id")
                     ? json_object_get_member(req, "id") : NULL;
        JsonObject *params = json_object_has_member(req, "params")
                           ? json_object_get_object_member(req, "params") : NULL;

        if (g_strcmp0(method, "initialize") == 0) {
            JsonObject *res = json_object_new();
            json_object_set_string_member(res, "protocolVersion", "2024-11-05");
            JsonObject *caps = json_object_new();
            json_object_set_object_member(caps, "tools", json_object_new());
            json_object_set_object_member(res, "capabilities", caps);
            JsonObject *info = json_object_new();
            json_object_set_string_member(info, "name", "sermoia-mcp");
            json_object_set_string_member(info, "version", SERMOIA_MCP_VERSION);
            json_object_set_object_member(res, "serverInfo", info);
            send_response(id, res, NULL);
        } else if (g_strcmp0(method, "tools/list") == 0) {
            JsonObject *res = json_object_new();
            json_object_set_array_member(res, "tools", tools_list());
            send_response(id, res, NULL);
        } else if (g_strcmp0(method, "tools/call") == 0) {
            const char *name = (params && json_object_has_member(params, "name"))
                             ? json_object_get_string_member(params, "name") : "";
            JsonObject *targs = (params && json_object_has_member(params, "arguments"))
                              ? json_object_get_object_member(params, "arguments") : NULL;
            send_response(id, tool_call(name, targs), NULL);
        } else if (id) {
            /* méthode inconnue avec id → erreur ; notifications (sans id) ignorées */
            send_response(id, NULL, "méthode non supportée");
        }
        g_object_unref(p);
    }
    g_string_free(line, TRUE);
    return 0;
}
