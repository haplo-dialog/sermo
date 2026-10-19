/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermoia-mcp.c — serveur MCP pour sermo : la boucle IA
 *   apprendre → générer → vérifier → (voir) → corriger.
 *
 * Réécriture CLEAN-ROOM des CONCEPTS (schéma / check / render / run) — aucun
 * code repris d'ailleurs. Enveloppe les binaires sermo existants :
 *   - sermo_schema   : décrit le langage XML de sermo en JSON (l'IA l'apprend) ;
 *   - sermo_check    : analyse un XML (`--print-ir`), sans rien exécuter ;
 *   - sermo_render   : dessine le dialogue en PNG, commandes du XML BLOQUÉES ;
 *   - sermo_run      : affiche le dialogue à l'utilisateur, renvoie ses valeurs ;
 *   - sermo_examples : exemples validés du catalogue, cherchés par mots ;
 *   - sermo_vote     : registre (ajout seul) des exemples qui aident ou trompent.
 *
 * Protocole : JSON-RPC 2.0 sur stdio, un message par ligne (spec MCP).
 * Binaire sermo utilisé : $SERMO_BIN (défaut « sermo »).
 *
 * Copyright (C) 2026 haplo-dialog <devel@haplo-dialog.fr>. GPL-2.0-or-later.
 */
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Version : posée par CMake depuis le fichier VERSION du dépôt (une seule
 * source, voir VERSIONING.md) ; la valeur ci-dessous ne sert qu'hors CMake. */
#ifndef SERMOIA_MCP_VERSION
#define SERMOIA_MCP_VERSION "inconnue"
#endif

/* Une requête plus longue est refusée (erreur d'analyse) : un client fou ne doit
 * pas faire grossir la mémoire sans fin. Un dialogue sermo tient en quelques ko. */
#define LIGNE_MAX (8u * 1024u * 1024u)

/* Commande « interdite à tout » : posée dans SERMO_ALLOWED_CMDS, elle n'autorise
 * aucune commande réelle ; le cœur refuse alors aussi le repli /bin/sh -c
 * (safe_exec.c). Sert à sermo_check et sermo_render, qui ne doivent RIEN lancer. */
#define AUCUNE_COMMANDE "/sermoia/aucune-commande"

static const char *sermo_bin(void)
{
    const char *b = g_getenv("SERMO_BIN");
    return (b && *b) ? b : "sermo";
}

/* Délai en secondes lu dans l'environnement, borné à [1, max]. */
static guint delai_env(const char *nom, guint defaut, guint max)
{
    const char *v = g_getenv(nom);
    if (!v || !*v) return defaut;
    guint64 n = g_ascii_strtoull(v, NULL, 10);
    if (n < 1) return defaut;
    return n > max ? max : (guint)n;
}

/* ─── Schéma du langage (clean-room, depuis la doc/tables de sermo) ─────────── */
/* tests/verifie.sh compare les sept listes de widgets à l'analyseur du cœur. */
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
"\"sources_cibles\":{"
"\"regle\":\"Un <input X> lit un équipement ou un service décrit par une adresse tout-en-un (schéma, hôte, puis clés après ?, séparées par &amp; dans le XML) ; une <action X> écrit vers lui, seulement dans un <button> ou un <menuitem>. Chaque forme demande son programme d'aide (paquet sermo-source-X pour lire, sermo-cible-X pour écrire), installé dans /usr/libexec/sermo. Un paramètre inconnu est refusé.\","
"\"lecture\":["
"{\"balise\":\"input modbus\",\"adresse\":\"tcp://hôte[:port]?registre=N (ou rtu:///dev/ttyUSB0)\",\"paquet\":\"sermo-source-modbus\",\"exemple\":\"<input modbus>tcp://automate:502?esclave=1&amp;registre=1&amp;diviseur=10</input>\"},"
"{\"balise\":\"input opcua\",\"adresse\":\"opc.tcp://hôte:port?noeud=ns=1;s=nom\",\"paquet\":\"sermo-source-opcua\",\"exemple\":\"<input opcua>opc.tcp://serveur:4840?noeud=ns=1;s=pieces</input>\"},"
"{\"balise\":\"input mqtt\",\"adresse\":\"mqtt://hôte[:port]?sujet=… (mqtts:// chiffré, confiance= obligatoire)\",\"paquet\":\"sermo-source-mqtt\",\"exemple\":\"<input mqtt>mqtt://serveur:1883?sujet=maison/salon/temperature</input>\"},"
"{\"balise\":\"input mavlink\",\"adresse\":\"udp://[hôte]:port, tcp://hôte:port ou serie:///dev/ttyUSB0, ?champ=…\",\"paquet\":\"sermo-source-mavlink\",\"exemple\":\"<input mavlink>udp://:14550?champ=hauteur</input>\"},"
"{\"balise\":\"input media\",\"adresse\":\"media://NOM?donnee=etat|position|duree\",\"paquet\":\"sermo-source-media\",\"exemple\":\"<input media>media://salon?donnee=etat</input>\"}"
"],"
"\"ecriture\":["
"{\"balise\":\"action mqtt\",\"adresse\":\"mqtt://hôte?sujet=…&amp;valeur=…\",\"paquet\":\"sermo-cible-mqtt\",\"garde\":\"liste blanche ~/.config/sermo/cibles-mqtt (600) ou /etc/sermo/cibles-mqtt ; identifiants= refusés sur mqtt:// sauf clair=oui\",\"exemple\":\"<button><label>Allumer</label><action mqtt>mqtt://serveur?sujet=maison/salon/lampe&amp;valeur=on</action></button>\"},"
"{\"balise\":\"action mavlink\",\"adresse\":\"LIAISON?parametre=NOM&amp;valeur=…, ?ordre=rtl|land|terminaison ou ?mission=/chemin\",\"paquet\":\"sermo-cible-mavlink\",\"garde\":\"liste blanche ~/.config/sermo/cibles-mavlink (600) ou /etc/sermo/cibles-mavlink ; paramètre réglé drone désarmé seulement ; terminaison en deux appuis dans 5 s ; mission envoyée, jamais lancée ; pas de pilotage\",\"exemple\":\"<button><label>Retour au départ</label><action mavlink>tcp://127.0.0.1:5760?ordre=rtl</action></button>\"},"
"{\"balise\":\"action media\",\"adresse\":\"media://NOM?commande=lire&amp;source=…|pause|reprendre|arreter|aller|volume|muet|son\",\"paquet\":\"sermo-cible-media\",\"garde\":\"sources file://, http(s)://, rtsp(s)://, ou caméra v4l2:///dev/videoN (taille=320x240|640x480|1280x720|1920x1080, format=mjpeg|yuyv422 ; fenêtre « CAMÉRA ACTIVE » ; caméra occupée refusée) seulement ; bwrap obligatoire pour une source réseau ; sans bwrap, fichier local ou caméra lus sans cage, avec un message ; dans=$VAR (gtk3 X11, sinon fenêtre séparée)\",\"exemple\":\"<button><label>Lire</label><action media>media://salon?commande=lire&amp;source=file:///srv/sons/annonce.ogg</action></button>\"}"
"]"
"},"
"\"exemple_minimal\":\"<window title=\\\"Nom\\\"><vbox><text><label>Votre nom :</label></text><entry><variable>NOM</variable></entry><hbox><button cancel></button><button ok></button></hbox></vbox></window>\","
"\"sortie\":\"À la fermeture, chaque widget nommé émet NOM=\\\"valeur\\\" sur stdout (échappé pour eval), puis EXIT=\\\"…\\\" (le bouton cliqué). Préférer --do : les valeurs arrivent par l'environnement, jamais relues comme du code.\","
"\"regle\":\"Racine <window>. Widgets imbriqués dans des conteneurs. <variable> nomme la sortie. <action>ACTION:cible</action> ou commande. <input> lance une commande pour remplir un widget.\","
"\"voir\":\"sermoman-mcp sert le manuel complet (attributs de chaque widget). sermo_examples sert des dialogues validés.\""
"}";

/* ─── Lancer un processus, avec délai ─────────────────────────────────────────
 * GSubprocess : l'entrée standard de l'enfant est /dev/null (jamais le canal
 * MCP), ses sorties sont captées (jamais écrites sur le canal MCP). L'enfant
 * ouvre son propre groupe de processus : à l'échéance, tout le groupe est tué
 * (xvfb-run, Xvfb et le backend), pas seulement le premier processus. */
typedef struct {
    gboolean lance;        /* le processus a démarré */
    gboolean expire;       /* tué à l'échéance */
    gboolean sorti;        /* sorti normalement (pas tué par un signal) */
    gint     code;         /* code de sortie si `sorti` */
    gchar   *out, *err;    /* sorties captées, UTF-8 valide */
    gchar   *erreur;       /* message si le lancement a échoué */
} Resultat;

static void resultat_vider(Resultat *r)
{
    g_free(r->out); g_free(r->err); g_free(r->erreur);
    memset(r, 0, sizeof *r);
}

static void nouveau_groupe(gpointer data) { (void)data; setpgid(0, 0); }

typedef struct { gboolean fini; GBytes *out, *err; GPid pid; gboolean expire; } Attente;

static void communique_fin(GObject *src, GAsyncResult *res, gpointer data)
{
    Attente *a = data;
    g_subprocess_communicate_finish(G_SUBPROCESS(src), res, &a->out, &a->err, NULL);
    a->fini = TRUE;
}

static gboolean echeance(gpointer data)
{
    Attente *a = data;
    a->expire = TRUE;
    if (a->pid > 0) kill(-a->pid, SIGKILL);
    return G_SOURCE_REMOVE;
}

static gchar *octets_texte(GBytes *b)
{
    if (!b) return g_strdup("");
    gsize n = 0;
    const char *d = g_bytes_get_data(b, &n);
    return g_utf8_make_valid(d ? d : "", (gssize)n);
}

/* env : paires nom/valeur à poser (valeur NULL = retirer), terminées par NULL. */
static void lancer(const char *const *argv, const char *const *env, guint delai_s, Resultat *r)
{
    memset(r, 0, sizeof *r);
    GMainContext *ctx = g_main_context_new();
    g_main_context_push_thread_default(ctx);

    GSubprocessLauncher *l = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE |
                                                       G_SUBPROCESS_FLAGS_STDERR_PIPE);
    for (int i = 0; env && env[i]; i += 2) {
        if (env[i + 1]) g_subprocess_launcher_setenv(l, env[i], env[i + 1], TRUE);
        else            g_subprocess_launcher_unsetenv(l, env[i]);
    }
    g_subprocess_launcher_set_child_setup(l, nouveau_groupe, NULL, NULL);

    GError *e = NULL;
    GSubprocess *p = g_subprocess_launcher_spawnv(l, argv, &e);
    if (!p) {
        r->erreur = g_strdup(e ? e->message : "lancement impossible");
        g_clear_error(&e);
    } else {
        r->lance = TRUE;
        Attente a = { FALSE, NULL, NULL, 0, FALSE };
        const char *ident = g_subprocess_get_identifier(p);
        a.pid = ident ? (GPid)atoi(ident) : 0;
        GSource *t = g_timeout_source_new_seconds(delai_s);
        g_source_set_callback(t, echeance, &a, NULL);
        g_source_attach(t, ctx);
        g_subprocess_communicate_async(p, NULL, NULL, communique_fin, &a);
        while (!a.fini) g_main_context_iteration(ctx, TRUE);
        g_subprocess_wait(p, NULL, NULL);
        g_source_destroy(t);
        g_source_unref(t);
        /* le groupe peut garder des processus orphelins (Xvfb) : les solder */
        if (a.pid > 0) kill(-a.pid, SIGKILL);
        r->expire = a.expire;
        r->sorti = g_subprocess_get_if_exited(p);
        r->code = r->sorti ? g_subprocess_get_exit_status(p) : -1;
        r->out = octets_texte(a.out);
        r->err = octets_texte(a.err);
        if (a.out) g_bytes_unref(a.out);
        if (a.err) g_bytes_unref(a.err);
        g_object_unref(p);
    }
    g_object_unref(l);
    g_main_context_pop_thread_default(ctx);
    g_main_context_unref(ctx);
}

/* Les toolkits écrivent sur stderr des messages qui ne disent rien du XML
 * (« Gtk-Message: Failed to load module … », eldbus, libEGL…). On ne garde
 * que ce qui aide l'IA à se corriger. */
static gchar *diagnostic_utile(const char *err)
{
    static const char *bruit[] = { "Gtk-Message", "Gtk-WARNING", "xapp-gtk3-module",
        "libEGL", "eldbus", "ERR<", "dbus", "Fontconfig", "propagateSizeHints",
        "QStandardPaths", "MESA", "AT-SPI", "atk-bridge", NULL };
    GString *g = g_string_new(NULL);
    gchar **l = g_strsplit(err ? err : "", "\n", -1);
    for (int i = 0; l[i]; i++) {
        if (!*g_strstrip(l[i])) continue;
        gboolean b = FALSE;
        for (int k = 0; bruit[k] && !b; k++) if (strstr(l[i], bruit[k])) b = TRUE;
        if (!b) g_string_append_printf(g, "%s\n", l[i]);
    }
    g_strfreev(l);
    return g_string_free(g, FALSE);
}

/* Dossier privé (0700) pour le XML et l'image : pas de nom prévisible dans /tmp. */
static gchar *dossier_prive(void)
{
    return g_dir_make_tmp("sermoia-XXXXXX", NULL);
}

static void dossier_effacer(gchar *d, const char *const *fichiers)
{
    if (!d) return;
    for (int i = 0; fichiers && fichiers[i]; i++) {
        gchar *f = g_build_filename(d, fichiers[i], NULL);
        g_unlink(f);
        g_free(f);
    }
    g_rmdir(d);
    g_free(d);
}

static gchar *ecrire_xml(const char *dir, const char *xml)
{
    gchar *f = g_build_filename(dir, "dialogue.xml", NULL);
    if (!g_file_set_contents(f, xml ? xml : "", -1, NULL)) { g_free(f); return NULL; }
    return f;
}

/* ─── Résultats MCP ────────────────────────────────────────────────────────── */
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

static JsonObject *tool_image_result(const char *b64, const char *legende)
{
    JsonObject *res = json_object_new();
    JsonArray *content = json_array_new();
    JsonObject *item = json_object_new();
    json_object_set_string_member(item, "type", "image");
    json_object_set_string_member(item, "data", b64 ? b64 : "");
    json_object_set_string_member(item, "mimeType", "image/png");
    json_array_add_object_element(content, item);
    if (legende) {
        JsonObject *t = json_object_new();
        json_object_set_string_member(t, "type", "text");
        json_object_set_string_member(t, "text", legende);
        json_array_add_object_element(content, t);
    }
    json_object_set_array_member(res, "content", content);
    return res;
}

/* ─── sermo_check ──────────────────────────────────────────────────────────── */
/* `--print-ir` analyse sans afficher. Commandes bloquées par prudence (le cœur
 * ne lance pas <input> à l'analyse, mesuré le 2026-10-11 ; la garde coûte rien). */
static JsonObject *tool_check(const char *xml, gboolean avec_ir)
{
    gchar *dir = dossier_prive();
    gchar *f = dir ? ecrire_xml(dir, xml) : NULL;
    if (!f) { dossier_effacer(dir, NULL); return tool_text_result("Impossible d'écrire le XML dans un dossier temporaire.", TRUE); }
    gchar *fa = g_strdup_printf("--file=%s", f);
    const char *argv[] = { sermo_bin(), "--print-ir", fa, NULL };
    const char *env[] = { "SERMO_ALLOWED_CMDS", AUCUNE_COMMANDE, NULL };
    Resultat r;
    lancer(argv, env, delai_env("SERMOIA_CHECK_DELAI", 20, 120), &r);
    const char *fichiers[] = { "dialogue.xml", NULL };
    dossier_effacer(dir, fichiers);
    g_free(fa); g_free(f);

    JsonObject *res;
    if (!r.lance) {
        gchar *m = g_strdup_printf("ERREUR — sermo n'a pas pu être lancé (%s) : %s\n"
                                   "Vérifier SERMO_BIN.", sermo_bin(), r.erreur);
        res = tool_text_result(m, TRUE); g_free(m);
    } else if (r.expire) {
        res = tool_text_result("ERREUR — l'analyse n'a pas fini à temps (SERMOIA_CHECK_DELAI).", TRUE);
    } else {
        gchar *diag = diagnostic_utile(r.err);
        gboolean ok = r.sorti && r.code == 0;
        GString *g = g_string_new(NULL);
        if (ok) {
            int n = 0;
            for (const char *c = r.out; c && *c; c++) if (*c == '\n') n++;
            g_string_append_printf(g, "OK — le XML s'analyse (%d ligne(s) de représentation interne).\n", n);
            if (*diag) g_string_append_printf(g, "Remarques de l'analyseur :\n%s", diag);
            if (avec_ir) g_string_append_printf(g, "--- représentation interne ---\n%s", r.out);
        } else {
            g_string_append(g, "ERREUR — l'analyseur refuse ce XML.\n");
            g_string_append(g, *diag ? diag : "(aucun message de l'analyseur)\n");
            g_string_append(g, "Conseil : le numéro de ligne compte les lignes du XML envoyé ; "
                               "une balise inconnue ou non fermée est la cause la plus fréquente "
                               "(voir sermo_schema).\n");
        }
        res = tool_text_result(g->str, !ok);
        g_string_free(g, TRUE);
        g_free(diag);
    }
    resultat_vider(&r);
    return res;
}

/* ─── sermo_run ────────────────────────────────────────────────────────────── */
static gboolean port_sans_fenetre(const char *bin)
{
    gchar *b = g_path_get_basename(bin);
    gboolean r = g_str_has_prefix(b, "ncurses") || g_str_has_prefix(b, "html");
    g_free(b);
    return r;
}

static JsonObject *tool_run(const char *xml, guint delai)
{
    const char *bin = sermo_bin();
    if (!port_sans_fenetre(bin) && !g_getenv("DISPLAY") && !g_getenv("WAYLAND_DISPLAY"))
        return tool_text_result("ERREUR — aucun affichage (ni DISPLAY ni WAYLAND_DISPLAY) : "
            "sermo_run ouvre une vraie fenêtre devant l'utilisateur. Lancer le serveur depuis "
            "une session graphique, ou utiliser sermo_render pour voir le dialogue.", TRUE);
    gchar *dir = dossier_prive();
    gchar *f = dir ? ecrire_xml(dir, xml) : NULL;
    if (!f) { dossier_effacer(dir, NULL); return tool_text_result("Impossible d'écrire le XML dans un dossier temporaire.", TRUE); }
    gchar *fa = g_strdup_printf("--file=%s", f);
    const char *argv[] = { bin, fa, NULL };
    Resultat r;
    lancer(argv, NULL, delai, &r);
    const char *fichiers[] = { "dialogue.xml", NULL };
    dossier_effacer(dir, fichiers);
    g_free(fa); g_free(f);

    JsonObject *res;
    if (!r.lance) {
        gchar *m = g_strdup_printf("ERREUR — sermo n'a pas pu être lancé (%s) : %s", bin, r.erreur);
        res = tool_text_result(m, TRUE); g_free(m);
    } else if (r.expire) {
        gchar *m = g_strdup_printf("ERREUR — le dialogue n'a pas été fermé dans les %u s : il a été "
                                   "fermé de force, sans valeurs. Augmenter 'timeout' si "
                                   "l'utilisateur a besoin de plus de temps.", delai);
        res = tool_text_result(m, TRUE); g_free(m);
    } else {
        gchar *diag = diagnostic_utile(r.err);
        gboolean valeurs = r.out && *r.out;
        GString *g = g_string_new(NULL);
        if (valeurs) g_string_append(g, r.out);
        else         g_string_append(g, "(aucune valeur exportée)\n");
        if (!(r.sorti && r.code == 0))
            g_string_append_printf(g, "--- sermo a fini en erreur (code %d) ---\n", r.code);
        if (*diag && (!valeurs || !(r.sorti && r.code == 0)))
            g_string_append_printf(g, "--- diagnostic ---\n%s", diag);
        res = tool_text_result(g->str, !valeurs || !(r.sorti && r.code == 0));
        g_string_free(g, TRUE);
        g_free(diag);
    }
    resultat_vider(&r);
    return res;
}

/* ─── sermo_render ───────────────────────────────────────────────────────────
 * Deux voies : plateformes hors écran posées (qt6, efl1, sdl3), sinon
 * --render-png sous xvfb, le backend écrivant lui-même le PNG (gtk3, gtk4,
 * fltk1, lvgl). ncurses (terminal) et html (page) n'ont pas d'image : aperçu
 * structurel texte. TOUTES les commandes du XML sont bloquées : dessiner ne
 * doit rien lancer (avant le 2026-10-11, un <input> s'exécutait au rendu). */
static gboolean png_valide(const char *path, guint *l, guint *h)
{
    gchar *d = NULL; gsize n = 0; gboolean ok = FALSE;
    /* signature PNG puis bloc IHDR : largeur et hauteur en grand-boutien */
    if (g_file_get_contents(path, &d, &n, NULL) && n > 24 &&
        memcmp(d, "\x89PNG\r\n\x1a\n", 8) == 0 && memcmp(d + 12, "IHDR", 4) == 0) {
        const guchar *u = (const guchar *)d;
        *l = (guint)u[16] << 24 | (guint)u[17] << 16 | (guint)u[18] << 8 | u[19];
        *h = (guint)u[20] << 24 | (guint)u[21] << 16 | (guint)u[22] << 8 | u[23];
        /* qt6 a pu rendre 2×2 pixels (une liste déroulante seule) : refusé */
        ok = *l >= 16 && *h >= 16;
    }
    g_free(d);
    return ok;
}

static JsonObject *tool_render(const char *xml)
{
    const char *rbin = g_getenv("SERMO_RENDER_BIN");
    if (!rbin || !*rbin) rbin = sermo_bin();
    guint delai = delai_env("SERMOIA_RENDER_DELAI", 30, 300);

    gchar *dir = dossier_prive();
    gchar *f = dir ? ecrire_xml(dir, xml) : NULL;
    if (!f) { dossier_effacer(dir, NULL); return tool_text_result("Impossible d'écrire le XML dans un dossier temporaire.", TRUE); }
    gchar *png = g_build_filename(dir, "rendu.png", NULL);
    gchar *pa = g_strdup_printf("--render-png=%s", png);
    gchar *fa = g_strdup_printf("--file=%s", f);
    const char *env[] = { "SERMO_ALLOWED_CMDS", AUCUNE_COMMANDE,
                          "QT_QPA_PLATFORM", "offscreen", "SDL_VIDEODRIVER", "offscreen",
                          "ELM_ENGINE", "buffer", "SERMO_HTML_OUT", "/dev/null", NULL };
    guint l = 0, h = 0;
    gboolean image = FALSE, expire = FALSE;
    gchar *diag = NULL;

    const char *direct[] = { rbin, pa, fa, NULL };
    Resultat r;
    lancer(direct, env, delai, &r);
    expire = r.expire;
    image = png_valide(png, &l, &h);
    if (!image && !expire) {
        resultat_vider(&r);
        const char *xvfb[] = { "xvfb-run", "-a", "-s", "-screen 0 1600x1200x24", rbin, pa, fa, NULL };
        lancer(xvfb, env, delai, &r);
        expire = r.expire;
        image = png_valide(png, &l, &h);
    }
    if (!image) diag = diagnostic_utile(r.lance ? r.err : r.erreur);
    resultat_vider(&r);

    JsonObject *res;
    if (image) {
        gchar *data = NULL; gsize n = 0;
        g_file_get_contents(png, &data, &n, NULL);
        gchar *b64 = g_base64_encode((const guchar *)data, n);
        gchar *b = g_path_get_basename(rbin);
        gchar *leg = g_strdup_printf("Rendu par %s, %u×%u pixels. Commandes du XML bloquées pendant "
                                     "le rendu : un widget rempli par <input> paraît vide.", b, l, h);
        res = tool_image_result(b64, leg);
        g_free(leg); g_free(b); g_free(b64); g_free(data);
    } else {
        /* repli : la représentation interne, toujours exploitable */
        const char *argv[] = { sermo_bin(), "--print-ir", fa, NULL };
        const char *env2[] = { "SERMO_ALLOWED_CMDS", AUCUNE_COMMANDE, NULL };
        Resultat ir;
        lancer(argv, env2, delai_env("SERMOIA_CHECK_DELAI", 20, 120), &ir);
        gchar *b = g_path_get_basename(rbin);
        GString *g = g_string_new(NULL);
        if (port_sans_fenetre(rbin))
            g_string_append_printf(g, "Pas d'image : %s n'ouvre pas de fenêtre (terminal ou page HTML). "
                                      "Aperçu STRUCTUREL à la place.\n", b);
        else if (expire)
            g_string_append_printf(g, "Pas d'image : le rendu par %s n'a pas fini dans les %u s "
                                      "(SERMOIA_RENDER_DELAI). Aperçu STRUCTUREL à la place.\n", b, delai);
        else
            g_string_append_printf(g, "Pas d'image : %s n'a pas produit de PNG (xvfb-run installé ? "
                                      "XML valide ? voir sermo_check). Aperçu STRUCTUREL à la place.\n", b);
        if (diag && *diag) g_string_append_printf(g, "--- diagnostic du rendu ---\n%s", diag);
        g_string_append_printf(g, "--- représentation interne (widgets/attributs) ---\n%s",
                               (ir.lance && ir.out && *ir.out) ? ir.out : "(indisponible)\n");
        res = tool_text_result(g->str, FALSE);
        g_string_free(g, TRUE); g_free(b);
        resultat_vider(&ir);
    }
    const char *fichiers[] = { "dialogue.xml", "rendu.png", NULL };
    dossier_effacer(dir, fichiers);
    g_free(diag); g_free(pa); g_free(fa); g_free(png); g_free(f);
    return res;
}

/* ─── sermo_examples : exemples validés, cherchés par mots ───────────────────
 * SERMO_EXAMPLES_DIR : un sous-dossier par sujet (button, formulaire…), des
 * dialogues .xml dedans. Une demande en phrase (« formulaire de connexion »)
 * est découpée en mots ; un sujet marque des points par mot trouvé dans son
 * nom (×3) et dans ses fichiers (×1) ; le vote départage. 100 % en C, sans
 * shell : `task` ne peut rien injecter. */
/* Catalogue : SERMO_EXAMPLES_DIR, sinon celui du paquet (posé par CMake :
 * <datadir>/sermoia-mcp/examples) s'il existe, sinon « examples » (lancé
 * depuis le dépôt). Installé, le serveur trouvait « examples » relatif au
 * dossier courant : rien. */
#ifndef SERMOIA_EXAMPLES_DIR
#define SERMOIA_EXAMPLES_DIR "/usr/share/sermoia-mcp/examples"
#endif
static const char *examples_dir(void)
{
    const char *d = g_getenv("SERMO_EXAMPLES_DIR");
    if (d && *d) return d;
    if (g_file_test(SERMOIA_EXAMPLES_DIR, G_FILE_TEST_IS_DIR)) return SERMOIA_EXAMPLES_DIR;
    return "examples";
}

/* ─── Vote de priorité, AJOUT SEUL ────────────────────────────────────────────
 * Fichier TSV : sujet<TAB>±1<TAB>note. On n'efface JAMAIS un vote. */
static gchar *vote_file(void)
{
    const char *v = g_getenv("SERMO_VOTE_FILE");
    if (v && *v) return g_strdup(v);
    return g_build_filename(g_get_user_state_dir(), "sermoia", "votes.tsv", NULL);
}

static gboolean append_vote(const char *key, int delta, const char *note)
{
    gchar *vf = vote_file(), *dir = g_path_get_dirname(vf);
    g_mkdir_with_parents(dir, 0700);
    FILE *fp = fopen(vf, "a");
    gboolean ok = FALSE;
    if (fp) {
        gchar *n1 = g_strdelimit(g_strdup(note ? note : ""), "\t\r\n", ' ');
        ok = fprintf(fp, "%s\t%d\t%s\n", key, delta, n1) > 0;
        ok = (fclose(fp) == 0) && ok;
        g_free(n1);
    }
    g_free(dir); g_free(vf);
    return ok;
}

static GHashTable *scores_vote(void)
{
    GHashTable *t = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    gchar *vf = vote_file(), *data = NULL;
    if (g_file_get_contents(vf, &data, NULL, NULL)) {
        gchar **lines = g_strsplit(data, "\n", -1);
        for (int i = 0; lines[i]; i++) {
            gchar **f = g_strsplit(lines[i], "\t", 3);
            if (f[0] && *f[0] && f[1]) {
                int s = GPOINTER_TO_INT(g_hash_table_lookup(t, f[0])) + atoi(f[1]);
                g_hash_table_insert(t, g_strdup(f[0]), GINT_TO_POINTER(s));
            }
            g_strfreev(f);
        }
        g_strfreev(lines);
    }
    g_free(data); g_free(vf);
    return t;
}

/* Un sujet ou un nom d'exemple : lettres, chiffres, « - », « _ », « . ». */
static gboolean nom_sur(const char *s)
{
    if (!s || !*s || s[0] == '.') return FALSE;
    for (const char *c = s; *c; c++)
        if (!g_ascii_isalnum(*c) && *c != '-' && *c != '_' && *c != '.') return FALSE;
    return TRUE;
}

static gchar **mots_cles(const char *task)
{
    static const char *vides[] = { "de", "du", "des", "la", "le", "les", "un", "une", "et", "ou",
        "pour", "avec", "sans", "dans", "sur", "qui", "que", "dialogue", "fenetre", "fenêtre",
        "the", "a", "an", "and", "or", "for", "with", "of", "to", "dialog", "window", NULL };
    GPtrArray *a = g_ptr_array_new();
    gchar *c = g_utf8_casefold(task ? task : "", -1);
    gchar **m = g_regex_split_simple("[^\\p{L}\\p{N}]+", c, G_REGEX_DEFAULT, 0);
    for (int i = 0; m[i]; i++) {
        if (g_utf8_strlen(m[i], -1) < 3) continue;
        gboolean v = FALSE;
        for (int k = 0; vides[k] && !v; k++) if (strcmp(m[i], vides[k]) == 0) v = TRUE;
        if (!v) g_ptr_array_add(a, g_strdup(m[i]));
    }
    g_strfreev(m); g_free(c);
    g_ptr_array_add(a, NULL);
    return (gchar **)g_ptr_array_free(a, FALSE);
}

typedef struct { gchar *nom; int pertinence, vote; GPtrArray *fichiers; } Sujet;

static int sujet_cmp(const void *x, const void *y)
{
    const Sujet *a = x, *b = y;
    if (a->pertinence != b->pertinence) return b->pertinence - a->pertinence;
    if (a->vote != b->vote) return b->vote - a->vote;
    return g_strcmp0(a->nom, b->nom);
}

static int chaine_cmp(gconstpointer a, gconstpointer b)
{
    return g_strcmp0(*(const char *const *)a, *(const char *const *)b);
}

/* Un exemple trouvé : sa note propre (nom du sujet ×3, nom du fichier ×3,
 * contenu ×1, par mot). Jusqu'à la 2.8.1, les fichiers d'un sujet sortaient
 * dans l'ordre alphabétique sous la note du SUJET : dans une boîte à outils,
 * « fail2ban » montrait d'abord le site nginx et les certificats. */
typedef struct { const Sujet *s; gchar *fichier; int note; } Trouve;

static int trouve_cmp(gconstpointer x, gconstpointer y)
{
    const Trouve *a = *(Trouve *const *)x, *b = *(Trouve *const *)y;
    if (a->note != b->note) return b->note - a->note;
    if (a->s->vote != b->s->vote) return b->s->vote - a->s->vote;
    int c = g_strcmp0(a->s->nom, b->s->nom);
    return c ? c : g_strcmp0(a->fichier, b->fichier);
}

static void trouve_free(gpointer p)
{
    Trouve *t = p;
    g_free(t->fichier);
    g_free(t);
}

static JsonObject *tool_examples(const char *task, int limite)
{
    const char *root = examples_dir();
    GDir *d = g_dir_open(root, 0, NULL);
    if (!d) {
        gchar *m = g_strdup_printf("Catalogue introuvable (SERMO_EXAMPLES_DIR=%s).", root);
        JsonObject *r = tool_text_result(m, TRUE); g_free(m); return r;
    }
    gchar **mots = mots_cles(task);
    GHashTable *votes = scores_vote();
    GArray *subs = g_array_new(FALSE, TRUE, sizeof(Sujet));
    GHashTable *notes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);   /* « sujet/fichier » → note */
    const char *sub;
    while ((sub = g_dir_read_name(d))) {
        gchar *sp = g_build_filename(root, sub, NULL);
        GDir *sd = nom_sur(sub) && g_file_test(sp, G_FILE_TEST_IS_DIR) ? g_dir_open(sp, 0, NULL) : NULL;
        if (!sd) { g_free(sp); continue; }
        Sujet s = { g_strdup(sub), 0, GPOINTER_TO_INT(g_hash_table_lookup(votes, sub)),
                    g_ptr_array_new_with_free_func(g_free) };
        gchar *nomcf = g_utf8_casefold(sub, -1);
        for (int k = 0; mots[k]; k++) if (strstr(nomcf, mots[k])) s.pertinence += 3;
        g_free(nomcf);
        const char *f;
        while ((f = g_dir_read_name(sd))) {
            if (!g_str_has_suffix(f, ".xml") || !nom_sur(f)) continue;
            g_ptr_array_add(s.fichiers, g_strdup(f));
            if (!mots[0]) continue;
            int note = 0;
            gchar *ncf = g_utf8_casefold(f, -1);
            for (int k = 0; mots[k]; k++) if (strstr(ncf, mots[k])) note += 3;
            g_free(ncf);
            gchar *fp = g_build_filename(sp, f, NULL), *txt = NULL;
            if (g_file_get_contents(fp, &txt, NULL, NULL)) {
                gchar *cf = g_utf8_casefold(txt, -1);
                for (int k = 0; mots[k]; k++) if (strstr(cf, mots[k])) { s.pertinence += 1; note += 1; }
                g_free(cf);
            }
            g_free(txt); g_free(fp);
            g_hash_table_insert(notes, g_strdup_printf("%s/%s", sub, f), GINT_TO_POINTER(note));
        }
        g_dir_close(sd); g_free(sp);
        g_ptr_array_sort(s.fichiers, chaine_cmp);
        g_array_append_val(subs, s);
    }
    g_dir_close(d);
    qsort(subs->data, subs->len, sizeof(Sujet), sujet_cmp);

    GString *out = g_string_new(NULL);
    if (!mots[0]) {
        /* sans mot-clé : la carte du catalogue, pas des pages de XML */
        g_string_append_printf(out, "Catalogue (%u sujets). Demander un sujet avec 'task' "
                                    "(un mot ou une phrase : « formulaire de connexion »).\n", subs->len);
        for (guint i = 0; i < subs->len; i++) {
            Sujet *s = &g_array_index(subs, Sujet, i);
            g_string_append_printf(out, "- %s (%u exemple(s)", s->nom, s->fichiers->len);
            if (s->vote) g_string_append_printf(out, ", vote %+d", s->vote);
            g_string_append(out, ")\n");
        }
    } else {
        int montres = 0;
        /* Tous les exemples, notés un à un : bonus du nom de sujet + note propre. */
        GPtrArray *tr = g_ptr_array_new_with_free_func(trouve_free);
        for (guint i = 0; i < subs->len; i++) {
            Sujet *s = &g_array_index(subs, Sujet, i);
            int bonus = 0;
            gchar *nomcf = g_utf8_casefold(s->nom, -1);
            for (int k = 0; mots[k]; k++) if (strstr(nomcf, mots[k])) bonus += 3;
            g_free(nomcf);
            for (guint j = 0; j < s->fichiers->len; j++) {
                const char *fn = g_ptr_array_index(s->fichiers, j);
                gchar *cle = g_strdup_printf("%s/%s", s->nom, fn);
                int note = bonus + GPOINTER_TO_INT(g_hash_table_lookup(notes, cle));
                g_free(cle);
                if (note <= 0) continue;
                Trouve *t = g_new0(Trouve, 1);
                t->s = s; t->fichier = g_strdup(fn); t->note = note;
                g_ptr_array_add(tr, t);
            }
        }
        g_ptr_array_sort(tr, trouve_cmp);
        for (guint i = 0; i < tr->len && montres < limite; i++) {
            const Trouve *t = g_ptr_array_index(tr, i);
            const Sujet *s = t->s;
            {
                const char *fn = t->fichier;
                gchar *fp = g_build_filename(root, s->nom, fn, NULL), *txt = NULL; gsize n = 0;
                if (g_file_get_contents(fp, &txt, &n, NULL) && n > 0) {
                    g_string_append_printf(out, "===== %s/%s  [pertinence %d · vote %d] =====\n",
                                           s->nom, fn, t->note, s->vote);
                    g_string_append_len(out, txt, n > 4000 ? 4000 : (gssize)n);
                    if (n > 4000) g_string_append(out, "\n… (tronqué)");
                    g_string_append(out, "\n");
                    montres++;
                }
                g_free(txt); g_free(fp);
            }
        }
        g_ptr_array_unref(tr);
        if (!montres) {
            g_string_append_printf(out, "Aucun exemple pour « %s ». Sujets disponibles :", task);
            for (guint i = 0; i < subs->len; i++)
                g_string_append_printf(out, "%s %s", i ? "," : "", g_array_index(subs, Sujet, i).nom);
            g_string_append(out, ".\n");
        } else {
            g_string_append(out, "Après usage : sermo_vote (example=\"sujet/fichier\", helpful=true|false).\n");
        }
    }
    for (guint i = 0; i < subs->len; i++) {
        g_free(g_array_index(subs, Sujet, i).nom);
        g_ptr_array_unref(g_array_index(subs, Sujet, i).fichiers);
    }
    g_array_free(subs, TRUE);
    g_hash_table_unref(notes);
    g_hash_table_unref(votes);
    g_strfreev(mots);
    JsonObject *r = tool_text_result(out->str, FALSE);
    g_string_free(out, TRUE);
    return r;
}

/* ─── sermo_vote ───────────────────────────────────────────────────────────── */
static JsonObject *tool_vote(const char *example, gboolean helpful, const char *note)
{
    if (!example || !*example) return tool_text_result("Argument 'example' manquant.", TRUE);
    gchar **parts = g_strsplit(example, "/", 2);
    gchar *key = g_strdup(parts[0] ? parts[0] : "");
    g_strfreev(parts);
    gchar *sp = nom_sur(key) ? g_build_filename(examples_dir(), key, NULL) : NULL;
    if (!sp || !g_file_test(sp, G_FILE_TEST_IS_DIR)) {
        gchar *m = g_strdup_printf("Sujet inconnu : « %s ». Voter pour un sujet du catalogue "
                                   "(voir sermo_examples sans argument).", example);
        JsonObject *r = tool_text_result(m, TRUE);
        g_free(m); g_free(sp); g_free(key); return r;
    }
    g_free(sp);
    if (!append_vote(key, helpful ? +1 : -1, note)) {
        gchar *vf = vote_file();
        gchar *m = g_strdup_printf("Vote non écrit : %s n'est pas inscriptible.", vf);
        JsonObject *r = tool_text_result(m, TRUE);
        g_free(m); g_free(vf); g_free(key); return r;
    }
    GHashTable *v = scores_vote();
    int sc = GPOINTER_TO_INT(g_hash_table_lookup(v, key));
    g_hash_table_unref(v);
    gchar *m = g_strdup_printf("Vote enregistré (ajout seul) : %s %s → score cumulé %d. "
        "sermo_examples départage les sujets par ce score.",
        key, helpful ? "a aidé (+1)" : "a trompé (−1)", sc);
    JsonObject *r = tool_text_result(m, FALSE);
    g_free(m); g_free(key);
    return r;
}

/* ─── tools/list ───────────────────────────────────────────────────────────── */
static JsonObject *prop(JsonObject *props, const char *nom, const char *type, const char *desc)
{
    JsonObject *p = json_object_new();
    json_object_set_string_member(p, "type", type);
    json_object_set_string_member(p, "description", desc);
    json_object_set_object_member(props, nom, p);
    return p;
}

/* indications MCP (2025-03-26) : lecture seule, destructif, monde ouvert */
static JsonObject *outil(JsonArray *arr, const char *name, const char *titre, const char *desc,
                         gboolean lecture_seule, gboolean destructif, gboolean monde_ouvert)
{
    JsonObject *t = json_object_new();
    json_object_set_string_member(t, "name", name);
    json_object_set_string_member(t, "title", titre);
    json_object_set_string_member(t, "description", desc);
    JsonObject *schema = json_object_new();
    json_object_set_string_member(schema, "type", "object");
    json_object_set_object_member(schema, "properties", json_object_new());
    json_object_set_object_member(t, "inputSchema", schema);
    JsonObject *an = json_object_new();
    json_object_set_string_member(an, "title", titre);
    json_object_set_boolean_member(an, "readOnlyHint", lecture_seule);
    json_object_set_boolean_member(an, "destructiveHint", destructif);
    json_object_set_boolean_member(an, "openWorldHint", monde_ouvert);
    json_object_set_object_member(t, "annotations", an);
    json_array_add_object_element(arr, t);
    return schema;
}

static void requis(JsonObject *schema, const char *const *noms)
{
    JsonArray *req = json_array_new();
    for (int i = 0; noms[i]; i++) json_array_add_string_element(req, noms[i]);
    json_object_set_array_member(schema, "required", req);
}

static JsonObject *props_de(JsonObject *schema) { return json_object_get_object_member(schema, "properties"); }

static JsonArray *tools_list(void)
{
    JsonArray *a = json_array_new();
    const char *xml_seul[] = { "xml", NULL };
    const char *desc_xml = "Le dialogue sermo complet, racine <window>.";

    outil(a, "sermo_schema", "Langage sermo",
          "Décrit le langage XML de sermo en JSON : conteneurs, widgets par famille, balises de "
          "contenu, attributs communs, actions, signaux, un exemple minimal et la forme de la "
          "sortie. À lire une fois avant d'écrire un dialogue. Ne lance rien.",
          TRUE, FALSE, FALSE);

    JsonObject *s = outil(a, "sermo_check", "Vérifier un dialogue",
          "Analyse un XML sermo avec l'analyseur réel (--print-ir), sans afficher ni exécuter. "
          "Renvoie « OK — … » ou « ERREUR — … » avec la ligne et le message de l'analyseur, pour "
          "se corriger. À appeler après chaque modification.",
          TRUE, FALSE, FALSE);
    prop(props_de(s), "xml", "string", desc_xml);
    prop(props_de(s), "ir", "boolean", "true : joint la représentation interne (widgets et attributs lus). Défaut false.");
    requis(s, xml_seul);

    s = outil(a, "sermo_render", "Voir un dialogue",
          "Dessine le dialogue en PNG sans l'afficher à l'écran, pour vérifier la mise en page. "
          "Les commandes du XML (<input>, <action>) sont BLOQUÉES pendant le rendu : un widget "
          "rempli par une commande paraît vide. Sans image possible (port terminal ou HTML, rendu "
          "raté), renvoie un aperçu structurel texte.",
          TRUE, FALSE, FALSE);
    prop(props_de(s), "xml", "string", desc_xml);
    requis(s, xml_seul);

    s = outil(a, "sermo_run", "Montrer un dialogue à l'utilisateur",
          "Ouvre le dialogue dans une vraie fenêtre, attend que l'utilisateur le ferme et renvoie "
          "ses valeurs (lignes NOM=\"valeur\", puis EXIT=\"…\"). Les commandes du XML S'EXÉCUTENT "
          "(<input>, <action>) : ne l'appeler qu'avec l'accord de l'utilisateur, sur un XML "
          "vérifié. Demande un affichage.",
          FALSE, TRUE, TRUE);
    prop(props_de(s), "xml", "string", desc_xml);
    prop(props_de(s), "timeout", "integer", "Secondes laissées à l'utilisateur avant fermeture forcée (défaut 300, de 5 à 3600).");
    requis(s, xml_seul);

    s = outil(a, "sermo_examples", "Exemples validés",
          "Sert des dialogues XML VALIDÉS du catalogue. 'task' : un mot ou une phrase (« formulaire "
          "de connexion », « calendar », « liste ») ; les sujets sont classés par pertinence puis par "
          "vote. Sans 'task' : la liste des sujets.",
          TRUE, FALSE, FALSE);
    prop(props_de(s), "task", "string", "Ce que le dialogue doit faire, en mots (optionnel).");
    prop(props_de(s), "limit", "integer", "Nombre maximal d'exemples renvoyés (défaut 4, de 1 à 10).");

    s = outil(a, "sermo_vote", "Noter un exemple",
          "Signale qu'un exemple a AIDÉ (helpful=true) ou TROMPÉ (helpful=false). Registre en ajout "
          "seul ; sermo_examples départage ensuite les sujets par ce score.",
          FALSE, FALSE, FALSE);
    prop(props_de(s), "example", "string", "« sujet » ou « sujet/fichier.xml », tel que sermo_examples l'a donné.");
    prop(props_de(s), "helpful", "boolean", "true = a aidé, false = a trompé.");
    prop(props_de(s), "note", "string", "Pourquoi, en une phrase (optionnel).");
    const char *rv[] = { "example", "helpful", NULL };
    requis(s, rv);
    return a;
}

/* ─── tools/call ───────────────────────────────────────────────────────────── */
static const char *arg_chaine(JsonObject *args, const char *nom)
{
    if (!args || !json_object_has_member(args, nom)) return NULL;
    JsonNode *n = json_object_get_member(args, nom);
    return (JSON_NODE_HOLDS_VALUE(n) && json_node_get_value_type(n) == G_TYPE_STRING)
           ? json_node_get_string(n) : NULL;
}

static gboolean arg_bool(JsonObject *args, const char *nom, gboolean *val)
{
    if (!args || !json_object_has_member(args, nom)) return FALSE;
    JsonNode *n = json_object_get_member(args, nom);
    if (!JSON_NODE_HOLDS_VALUE(n) || json_node_get_value_type(n) != G_TYPE_BOOLEAN) return FALSE;
    *val = json_node_get_boolean(n);
    return TRUE;
}

static gint64 arg_entier(JsonObject *args, const char *nom, gint64 defaut, gint64 min, gint64 max)
{
    if (!args || !json_object_has_member(args, nom)) return defaut;
    JsonNode *n = json_object_get_member(args, nom);
    if (!JSON_NODE_HOLDS_VALUE(n)) return defaut;
    GType t = json_node_get_value_type(n);
    gint64 v = t == G_TYPE_INT64 ? json_node_get_int(n)
             : t == G_TYPE_DOUBLE ? (gint64)json_node_get_double(n) : defaut;
    return v < min ? min : v > max ? max : v;
}

/* NULL si l'outil est inconnu : erreur de protocole -32602 (spec MCP). */
static JsonObject *tool_call(const char *name, JsonObject *args)
{
    const char *xml = arg_chaine(args, "xml");
    gboolean besoin_xml = g_strcmp0(name, "sermo_check") == 0 || g_strcmp0(name, "sermo_render") == 0
                       || g_strcmp0(name, "sermo_run") == 0;
    if (besoin_xml && (!xml || !*xml))
        return tool_text_result("Argument 'xml' manquant ou vide (une chaîne : le dialogue, racine <window>).", TRUE);

    if (g_strcmp0(name, "sermo_schema") == 0) return tool_text_result(SCHEMA_JSON, FALSE);
    if (g_strcmp0(name, "sermo_check") == 0) {
        gboolean ir = FALSE;
        arg_bool(args, "ir", &ir);
        return tool_check(xml, ir);
    }
    if (g_strcmp0(name, "sermo_render") == 0) return tool_render(xml);
    if (g_strcmp0(name, "sermo_run") == 0)
        return tool_run(xml, (guint)arg_entier(args, "timeout", 300, 5, 3600));
    if (g_strcmp0(name, "sermo_examples") == 0)
        return tool_examples(arg_chaine(args, "task"), (int)arg_entier(args, "limit", 4, 1, 10));
    if (g_strcmp0(name, "sermo_vote") == 0) {
        gboolean helpful;
        if (!arg_bool(args, "helpful", &helpful))
            return tool_text_result("Argument 'helpful' manquant (true ou false).", TRUE);
        return tool_vote(arg_chaine(args, "example"), helpful, arg_chaine(args, "note"));
    }
    return NULL;
}

/* ─── JSON-RPC ─────────────────────────────────────────────────────────────── */
enum { ERR_ANALYSE = -32700, ERR_REQUETE = -32600, ERR_METHODE = -32601, ERR_PARAMS = -32602 };

static void envoyer(JsonObject *root)
{
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

static void repondre(JsonNode *id, JsonObject *result)
{
    JsonObject *root = json_object_new();
    json_object_set_string_member(root, "jsonrpc", "2.0");
    json_object_set_member(root, "id", json_node_copy(id));
    json_object_set_object_member(root, "result", result ? result : json_object_new());
    envoyer(root);
}

static void erreur(JsonNode *id, int code, const char *msg)
{
    JsonObject *root = json_object_new();
    json_object_set_string_member(root, "jsonrpc", "2.0");
    if (id) json_object_set_member(root, "id", json_node_copy(id));
    else    json_object_set_null_member(root, "id");
    JsonObject *e = json_object_new();
    json_object_set_int_member(e, "code", code);
    json_object_set_string_member(e, "message", msg);
    json_object_set_object_member(root, "error", e);
    envoyer(root);
}

/* Versions du protocole que ce serveur sait parler (outils seulement : les
 * trois sont compatibles pour lui). On renvoie celle du client si on la
 * connaît, sinon la plus récente. */
static const char *VERSIONS[] = { "2025-06-18", "2025-03-26", "2024-11-05", NULL };

static const char *INSTRUCTIONS =
    "sermoia aide à écrire des dialogues sermo (XML, racine <window>). Boucle conseillée : "
    "1) sermo_schema une fois ; 2) sermo_examples avec ce que le dialogue doit faire ; "
    "3) écrire le XML ; 4) sermo_check, corriger jusqu'à « OK » ; 5) sermo_render pour voir "
    "la mise en page ; 6) sermo_run seulement si l'utilisateur veut l'essayer — c'est le seul "
    "outil qui exécute les commandes du XML. Après usage d'un exemple, sermo_vote.";

static void traiter(const char *ligne, gsize n)
{
    JsonParser *p = json_parser_new();
    if (!json_parser_load_from_data(p, ligne, (gssize)n, NULL)) {
        erreur(NULL, ERR_ANALYSE, "JSON illisible");
        g_object_unref(p);
        return;
    }
    JsonNode *rootn = json_parser_get_root(p);
    if (!rootn || !JSON_NODE_HOLDS_OBJECT(rootn)) {
        erreur(NULL, ERR_REQUETE, "requête invalide : un objet JSON-RPC est attendu (les lots ne sont pas pris en charge)");
        g_object_unref(p);
        return;
    }
    JsonObject *req = json_node_get_object(rootn);
    JsonNode *id = json_object_has_member(req, "id") ? json_object_get_member(req, "id") : NULL;
    const char *method = arg_chaine(req, "method");
    JsonNode *pn = json_object_has_member(req, "params") ? json_object_get_member(req, "params") : NULL;
    JsonObject *params = (pn && JSON_NODE_HOLDS_OBJECT(pn)) ? json_node_get_object(pn) : NULL;

    if (!method) {
        if (id) erreur(id, ERR_REQUETE, "requête invalide : 'method' manque");
    } else if (!id) {
        /* notification (notifications/initialized, cancelled…) : pas de réponse */
    } else if (strcmp(method, "initialize") == 0) {
        const char *demandee = arg_chaine(params, "protocolVersion");
        const char *v = VERSIONS[0];
        for (int i = 0; VERSIONS[i]; i++) if (g_strcmp0(demandee, VERSIONS[i]) == 0) v = VERSIONS[i];
        JsonObject *res = json_object_new();
        json_object_set_string_member(res, "protocolVersion", v);
        JsonObject *caps = json_object_new();
        json_object_set_object_member(caps, "tools", json_object_new());
        json_object_set_object_member(res, "capabilities", caps);
        JsonObject *info = json_object_new();
        json_object_set_string_member(info, "name", "sermoia-mcp");
        json_object_set_string_member(info, "title", "sermoia — écrire, vérifier et voir des dialogues sermo");
        json_object_set_string_member(info, "version", SERMOIA_MCP_VERSION);
        json_object_set_object_member(res, "serverInfo", info);
        json_object_set_string_member(res, "instructions", INSTRUCTIONS);
        repondre(id, res);
    } else if (strcmp(method, "ping") == 0) {
        repondre(id, json_object_new());
    } else if (strcmp(method, "tools/list") == 0) {
        JsonObject *res = json_object_new();
        json_object_set_array_member(res, "tools", tools_list());
        repondre(id, res);
    } else if (strcmp(method, "tools/call") == 0) {
        const char *name = arg_chaine(params, "name");
        JsonNode *an = params && json_object_has_member(params, "arguments")
                     ? json_object_get_member(params, "arguments") : NULL;
        JsonObject *targs = (an && JSON_NODE_HOLDS_OBJECT(an)) ? json_node_get_object(an) : NULL;
        JsonObject *res = name ? tool_call(name, targs) : NULL;
        if (res) repondre(id, res);
        else {
            gchar *m = g_strdup_printf("outil inconnu : %s", name ? name : "(sans nom)");
            erreur(id, ERR_PARAMS, m);
            g_free(m);
        }
    } else {
        gchar *m = g_strdup_printf("méthode non prise en charge : %s", method);
        erreur(id, ERR_METHODE, m);
        g_free(m);
    }
    g_object_unref(p);
}

/* ─── Boucle principale : un message JSON-RPC par ligne sur stdin ──────────── */
int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGPIPE, SIG_IGN);
    GString *line = g_string_new(NULL);
    gboolean trop_long = FALSE;
    int c;
    while ((c = getc(stdin)) != EOF) {
        if (c != '\n') {
            if (line->len < LIGNE_MAX) g_string_append_c(line, (gchar)c);
            else trop_long = TRUE;
            continue;
        }
        if (trop_long) erreur(NULL, ERR_ANALYSE, "message trop long (8 Mio au plus)");
        else if (line->len > 0 && !(line->len == 1 && line->str[0] == '\r'))
            traiter(line->str, line->len);
        g_string_set_size(line, 0);
        trop_long = FALSE;
    }
    g_string_free(line, TRUE);
    return 0;
}
