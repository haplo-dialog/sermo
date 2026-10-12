/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* render_html.c — exporteur HTML statique du backend `htmlsermo`.
 *
 * Reprend le modèle « arbre de WidgetNode » toolkit-agnostique du port neutre
 * (partagé avec sdl3/ncurses), mais au lieu de dessiner à l'écran ou dans un
 * terminal, sérialise l'arbre en UNE page HTML autonome (CSS embarqué, aucune
 * dépendance externe). Trois modes (SERMO_HTML_MODE, voir plus bas) : page
 * morte (défaut), page vivante sans réseau, ou page servie en local qui renvoie
 * les valeurs saisies (serve_http.c). README.md du port pour l'usage.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render_html.h"
#include "dialog_state.h"

/* --space-expand / --space-fill du programme (cœur, gtkdialog.c) : -1 = non dit. */
extern gint project_space_expand;
extern gint project_space_fill;

/* ─── Échappement HTML ───────────────────────────────────────────────────── */
static void html_escape(FILE *out, const char *s)
{
    if (!s) return;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        switch (*p) {
        case '&': fputs("&amp;", out);  break;
        case '<': fputs("&lt;", out);   break;
        case '>': fputs("&gt;", out);   break;
        case '"': fputs("&quot;", out); break;
        case '\'':fputs("&#39;", out);  break;
        default:  fputc(*p, out);
        }
    }
}

/* ─── Mode de rendu ───────────────────────────────────────────────────────────
 * SERMO_HTML_MODE choisit ce que la page sait faire :
 *
 *   export       (défaut) page morte : champs verrouillés, aucun JavaScript.
 *                C'est le comportement historique du port, inchangé.
 *   interactive  page vivante SANS réseau : les champs se saisissent, un bout
 *                de JavaScript compose le bloc VAR="valeur" et permet de le
 *                copier ou de le télécharger. L'humain reporte le résultat.
 *   serve        les champs se saisissent et le formulaire est POSTé au
 *                micro-serveur local qui a servi la page ; c'est lui qui
 *                imprime VAR="valeur" sur la sortie standard.
 *
 * Le drapeau est une variable d'environnement et non une option de ligne de
 * commande parce que htmlsermo n'a pas de main() à lui : il vient du cœur, lié
 * statiquement, et le cœur REFUSE toute option qu'il ne connaît pas. Ajouter
 * « --serve » supposerait de toucher libsermocore, partagé par les neuf ports,
 * pour y mettre un drapeau propre à un seul. */
#define MODE_EXPORT      0
#define MODE_INTERACTIVE 1
#define MODE_SERVE       2

int html_mode(void)
{
    static int cached = -1;
    if (cached >= 0) return cached;
    const char *m = getenv("SERMO_HTML_MODE");
    if      (m && !strcmp(m, "interactive")) cached = MODE_INTERACTIVE;
    else if (m && !strcmp(m, "serve"))       cached = MODE_SERVE;
    else                                     cached = MODE_EXPORT;
    return cached;
}

/* En mode export les champs restent verrouillés ; sinon ils se saisissent. */
static const char *ro(void)  { return html_mode() == MODE_EXPORT ? " readonly" : ""; }
static const char *dis(void) { return html_mode() == MODE_EXPORT ? " disabled" : ""; }

/* Nom du champ dans le formulaire = nom de la variable du dialogue. Sans lui,
 * ni le JavaScript ni le serveur ne sauraient à quoi rattacher la valeur. */
static void fld_name(FILE *out, const WidgetNode *n)
{
    if (html_mode() == MODE_EXPORT) return;
    if (!n || !n->var_name || !*n->var_name) return;
    fputs(" name=\"", out); html_escape(out, n->var_name); fputc('"', out);
}

static const char *safe_label(const WidgetNode *n)
{
    return (n && n->label) ? n->label : "";
}

/* ─── Images : inclusion en data: URI ─────────────────────────────────────────
 * La promesse du port est « UNE page autonome, aucun fichier externe ». Une
 * image ne peut donc pas etre referencee par son chemin : elle doit entrer
 * DANS la page. On l'encode en base64 et on l'ecrit en data: URI.
 *
 * Contrainte : ce backend ne lie PAS glib (html-compat.h est un shim autonome,
 * c'est ce qui lui donne son « Depends: libc6 » seul). Pas de g_base64_encode —
 * l'encodeur tient en vingt lignes et le port garde sa propriete de n'avoir
 * aucune dependance.
 *
 * Garde-fou : au-dela de SERMO_HTML_IMG_MAX octets (defaut 1 Mio) on retombe
 * sur le pictogramme plutot que de fabriquer une page de 40 Mio. Idem si le
 * fichier est absent, illisible, ou d'un type inconnu. */

#define IMG_MAX_DEFAULT (1024L * 1024L)

static long img_max_bytes(void)
{
    const char *v = getenv("SERMO_HTML_IMG_MAX");
    if (v && *v) { char *e; long n = strtol(v, &e, 10); if (e != v && n >= 0) return n; }
    return IMG_MAX_DEFAULT;
}

static int ascii_casecmp(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        int ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
        int cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if (ca != cb) return ca - cb;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

/* Type MIME deduit de l'extension. NULL = type non gere, on n'inclut pas. */
static const char *img_mime(const char *path)
{
    const char *dot = path ? strrchr(path, '.') : NULL;
    if (!dot) return NULL;
    if (!ascii_casecmp(dot, ".png"))  return "image/png";
    if (!ascii_casecmp(dot, ".jpg") || !ascii_casecmp(dot, ".jpeg")) return "image/jpeg";
    if (!ascii_casecmp(dot, ".gif"))  return "image/gif";
    if (!ascii_casecmp(dot, ".webp")) return "image/webp";
    if (!ascii_casecmp(dot, ".svg"))  return "image/svg+xml";
    if (!ascii_casecmp(dot, ".bmp"))  return "image/bmp";
    if (!ascii_casecmp(dot, ".ico"))  return "image/x-icon";
    return NULL;
}

/* Ecrit `buf` en base64 directement dans `out` : pas de tampon intermediaire,
 * une image de 1 Mio couterait 1,4 Mio de plus en memoire. */
static void b64_write(FILE *out, const unsigned char *buf, size_t len)
{
    static const char T[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t i = 0;
    for (; i + 2 < len; i += 3) {
        unsigned v = (unsigned)buf[i] << 16 | (unsigned)buf[i+1] << 8 | buf[i+2];
        fputc(T[(v >> 18) & 63], out); fputc(T[(v >> 12) & 63], out);
        fputc(T[(v >>  6) & 63], out); fputc(T[ v        & 63], out);
    }
    if (i < len) {                       /* reste : 1 ou 2 octets */
        unsigned v = (unsigned)buf[i] << 16;
        if (i + 1 < len) v |= (unsigned)buf[i+1] << 8;
        fputc(T[(v >> 18) & 63], out);
        fputc(T[(v >> 12) & 63], out);
        fputc(i + 1 < len ? T[(v >> 6) & 63] : '=', out);
        fputc('=', out);
    }
}

/* Tente d'ecrire <img src="data:..."> pour `path`. Renvoie 0 si l'image n'a pas
 * pu etre incluse : l'appelant retombe alors sur le pictogramme.
 * w, h : taille demandée en pixels (0 = libre) ; l'image garde son ratio
 * (object-fit:contain), comme gtkdialog qui la charge « at_size ». Une icône
 * SYMBOLIQUE (« …-symbolic.svg ») prend la couleur du texte, comme dans GTK :
 * elle sert de masque à un carré de cette couleur. */
static int html_emit_image(FILE *out, const char *path, const char *cls,
                           const char *alt, int w, int h)
{
    if (!path || !*path) return 0;

    /* Le cœur préfixe la valeur pour dire d'où elle vient (cf. gtkdialog) :
     *   « File:/chemin »  → un fichier ; « File: » seul → icône de thème ;
     *   « Command:…    »  → le chemin sort d'une commande à exécuter.
     * On ne traite QUE le premier cas. Une commande n'est JAMAIS lancée pour
     * résoudre une image : exporter une page ne doit pas exécuter le système.
     * Tout le reste retombe sur le pictogramme. */
    if (!strncmp(path, "Command:", 8)) return 0;
    if (!strncmp(path, "File:", 5))    path += 5;
    if (!*path) return 0;               /* « File: » seul : icône de thème */

    const char *mime = img_mime(path);
    if (!mime) return 0;

    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long sz = ftell(f);
    long max = img_max_bytes();
    if (sz <= 0 || (max > 0 && sz > max)) { fclose(f); return 0; }
    rewind(f);

    unsigned char *buf = (unsigned char *)malloc((size_t)sz);
    if (!buf) { fclose(f); return 0; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(buf); return 0; }

    int symbolique = strstr(path, "-symbolic.") && !strcmp(mime, "image/svg+xml");
    if (symbolique) {
        int cote = w > 0 ? w : (h > 0 ? h : 16);
        fputs("<span class=\"", out); html_escape(out, cls);
        fputs(" sermo-symbolique\" role=\"img\" aria-label=\"", out); html_escape(out, alt ? alt : "");
        fprintf(out, "\" style=\"width:%dpx;height:%dpx;", cote, h > 0 ? h : cote);
        for (int k = 0; k < 2; k++) {
            fputs(k ? "mask-image:url(data:" : "-webkit-mask-image:url(data:", out);
            fputs(mime, out); fputs(";base64,", out);
            b64_write(out, buf, got);
            fputs(");", out);
        }
        fputs("\"></span>", out);
        free(buf);
        return 1;
    }
    fputs("<img class=\"", out); html_escape(out, cls);
    fputs("\" alt=\"", out);    html_escape(out, alt ? alt : "");
    if (w > 0 || h > 0) {
        fputs("\" style=\"", out);
        if (w > 0) fprintf(out, "width:%dpx;", w);
        if (h > 0) fprintf(out, "height:%dpx;", h);
        fputs("object-fit:contain;", out);
    }
    fputs("\" src=\"data:", out); fputs(mime, out); fputs(";base64,", out);
    b64_write(out, buf, got);
    fputs("\">", out);
    free(buf);
    return 1;
}

/* ─── Rendu récursif ──────────────────────────────────────────────────────
 * Chaque nœud devient un élément HTML sémantique proche de son équivalent
 * natif. Les feuilles interactives sont rendues en lecture seule/désactivées
 * (export statique, pas de logique serveur derrière) — voir doc du module. */
static void render_node(FILE *out, WidgetNode *n, int depth);

/* Les enfants d'une boîte, rangés comme gtkdialog (widget_hbox.c et
 * widget_vbox.c de gtk3sermo, l'étalon) :
 *
 *  - Étirement (expand) et remplissage (fill). La valeur COURANTE part de
 *    celle de la boîte (<vbox space-expand=… space-fill=…>, sinon l'option
 *    --space-expand/--space-fill du programme, sinon « non dit ») ; on
 *    parcourt les enfants — colonne dans l'ordre du document, rangée À
 *    L'ENVERS — et un enfant qui a son propre réglage REMPLACE la valeur
 *    courante pour lui ET pour ceux parcourus après lui (l'étalon ne la remet
 *    jamais à zéro). « Non dit » : le défaut du type — <edit>, <frame>
 *    s'étirent partout, plus <entry> dans une rangée, <paned> dans une
 *    colonne. Jusqu'à la 2.8.1, seul le réglage propre de l'enfant comptait :
 *    une boîte qui demandait l'étirement n'était pas suivie.
 *  - Un enfant étiré qui ne remplit pas sa place y est CENTRÉ.
 *  - homogeneous="true" : chaque enfant reçoit une case de MÊME taille et
 *    l'ensemble occupe toute la boîte ; un enfant qui ne remplit pas sa case
 *    y est centré. Jusqu'à la 2.8.1, l'attribut était ignoré (rangée serrée
 *    à droite). */
static int bool_gtk(int v) { return v == 1 ? 1 : v == -1 ? 0 : -1; }

static int defaut_etire(const WidgetNode *c, int horizontal)
{
    if (c->type == WT_EDIT || c->type == WT_FRAME) return 1;
    return horizontal ? c->type == WT_ENTRY : c->type == WT_PANED;
}

/* Aussi pour les enfants d'un <frame> : l'étalon (widget_frame.c) les range
 * dans une colonne cachée, dont la valeur courante part de l'option du
 * programme SEULEMENT — le space-expand du cadre vaut pour le cadre dans sa
 * boîte, pas pour ses enfants. Défaut : <edit>, <frame>, <paned> s'étirent. */
static void render_enfants_boite(FILE *out, WidgetNode *n, int depth)
{
    const int horizontal = n->type == WT_HBOX;
    const int cadre = n->type == WT_FRAME || n->type == WT_ASPECTFRAME;
    const int nb = n->child_count;
    if (nb <= 0) return;
    unsigned char *etire = calloc((size_t)nb, 1), *remplit = calloc((size_t)nb, 1);
    if (!etire || !remplit) {
        free(etire); free(remplit);
        for (int i = 0; i < nb; i++) render_node(out, n->children[i], depth + 1);
        return;
    }
    /* -1 = non dit, 0 = faux, 1 = vrai (comme space_expand de l'étalon) */
    int cur_e = cadre ? -1 : bool_gtk(n->expand), cur_f = cadre ? -1 : bool_gtk(n->fill);
    if (cur_e == -1 && project_space_expand != -1) cur_e = project_space_expand ? 1 : 0;
    if (cur_f == -1 && project_space_fill != -1)   cur_f = project_space_fill ? 1 : 0;
    for (int k = 0; k < nb; k++) {
        int i = horizontal ? nb - 1 - k : k;
        WidgetNode *c = n->children[i];
        if (!c) continue;
        if (c->expand) cur_e = bool_gtk(c->expand);
        if (c->fill)   cur_f = bool_gtk(c->fill);
        int d = defaut_etire(c, horizontal);
        etire[i]   = (unsigned char)(cur_e != -1 ? cur_e : d);
        remplit[i] = (unsigned char)(cur_f != -1 ? cur_f : d);
    }
    for (int i = 0; i < nb; i++) {
        WidgetNode *c = n->children[i];
        if (!c || !c->visible) continue;
        if ((n->homogene && !cadre) || etire[i]) {
            fprintf(out, "<div class=\"%s%s\">\n",
                    n->homogene && !cadre ? "sermo-case" : "sermo-etire",
                    remplit[i] ? "" : " sermo-centre");
            render_node(out, c, depth + 1);
            fputs("</div>\n", out);
        } else
            render_node(out, c, depth + 1);
    }
    free(etire); free(remplit);
}

static void render_node(FILE *out, WidgetNode *n, int depth)
{
    if (!n || !n->visible) return;
    const char *lbl = safe_label(n);

    switch (n->type) {

    case WT_WINDOW: {
        /* default-width / width-request : la fenêtre a cette largeur (bornée par
         * l'écran), comme une vraie fenêtre ; jusqu'à la 2.8.1, 640 px fixes —
         * une boîte horizontale plus large passait alors à la ligne. */
        if (n->width > 0 || n->height > 0) {
            fputs("<main class=\"sermo-window\" style=\"", out);
            if (n->width > 0)  fprintf(out, "width:%dpx;max-width:100%%;", n->width);
            if (n->height > 0) fprintf(out, "min-height:%dpx;", n->height);
            fputs("\">\n", out);
        } else
            fputs("<main class=\"sermo-window\">\n", out);
        if (*lbl) { fputs("<h1>", out); html_escape(out, lbl); fputs("</h1>\n", out); }
        for (int i = 0; i < n->child_count; i++) render_node(out, n->children[i], depth + 1);
        fputs("</main>\n", out);
        return;
    }

    case WT_EVENTBOX:
        /* Zone qui entoure VRAIMENT son enfant (le stub le perdait). Le title
         * porte l'action éventuelle : une page HTML ne lance pas de commande. */
        fputs("<div class=\"sermo-eventbox\"", out);
        if (n->action && *n->action) {
            fputs(" title=\"action : ", out); html_escape(out, n->action); fputc('"', out);
        }
        fputs(">\n", out);
        for (int i = 0; i < n->child_count; i++) render_node(out, n->children[i], depth + 1);
        fputs("</div>\n", out);
        return;

    case WT_PANED: {
        /* Deux zones en flex. La poignée est le « resize » CSS de la première
         * zone : elle se traîne VRAIMENT à la souris, sans une ligne de
         * JavaScript — la page reste autonome jusque dans l'export statique.
         * C'est le seul endroit du port où l'utilisateur peut encore agir sur
         * une page morte, et c'est gratuit. */
        int vert = n->state.paned.vertical;
        int pct  = (int) (n->state.paned.fraction * 100);
        if (pct < 5) pct = 5;
        if (pct > 95) pct = 95;
        fprintf(out, "<div class=\"sermo-paned\" style=\"flex-direction:%s;\">\n",
                vert ? "column" : "row");
        if (n->child_count > 0) {
            fprintf(out, "<div class=\"sermo-paned-part\" style=\"flex:0 0 %d%%;resize:%s;\">\n",
                    pct, n->state.paned.resizable ? (vert ? "vertical" : "horizontal") : "none");
            render_node(out, n->children[0], depth + 1);
            fputs("</div>\n", out);
        }
        fputs("<div class=\"sermo-paned-poignee\"></div>\n", out);
        if (n->child_count > 1) {
            fputs("<div class=\"sermo-paned-part\" style=\"flex:1 1 auto;\">\n", out);
            render_node(out, n->children[1], depth + 1);
            fputs("</div>\n", out);
        }
        fputs("</div>\n", out);
        return;
    }

    case WT_WIZARD: {
        /* Les étapes sont toutes écrites, une seule visible ; la navigation
         * est affichée mais INERTE dans l'export statique — une page morte ne
         * change pas d'étape, et le dire vaut mieux que de le cacher. */
        int cur = n->state.notebook.current_tab;
        if (cur < 0 || cur >= n->child_count) cur = 0;
        fputs("<div class=\"sermo-wizard\">\n", out);
        for (int i = 0; i < n->child_count; i++) {
            fprintf(out, "<div class=\"sermo-stack-page\"%s>\n", i == cur ? "" : " hidden");
            render_node(out, n->children[i], depth + 1);
            fputs("</div>\n", out);
        }
        fprintf(out, "<nav class=\"sermo-toolbar\"><button class=\"sermo-button\" type=\"button\"%s>Précédent</button>"
                     "<button class=\"sermo-button\" type=\"button\"%s>Suivant</button>"
                     "<button class=\"sermo-button\" type=\"button\"%s>Terminer</button>"
                     "<span class=\"sermo-text\">étape %d/%d</span></nav>\n",
                html_mode() == MODE_EXPORT ? " disabled" : "",
                html_mode() == MODE_EXPORT ? " disabled" : "",
                html_mode() == MODE_EXPORT ? " disabled" : "",
                cur + 1, n->child_count);
        fputs("</div>\n", out);
        return;
    }

    case WT_STACK: {
        /* Les pages non visibles sont écrites mais masquées : la page produite
         * reste complète (on peut la lire, la chercher), et seule la page
         * courante s'affiche. */
        int cur = n->state.notebook.current_tab;
        if (cur < 0 || cur >= n->child_count) cur = 0;
        fputs("<div class=\"sermo-stack\">\n", out);
        for (int i = 0; i < n->child_count; i++) {
            fprintf(out, "<div class=\"sermo-stack-page\"%s>\n",
                    i == cur ? "" : " hidden");
            render_node(out, n->children[i], depth + 1);
            fputs("</div>\n", out);
        }
        fputs("</div>\n", out);
        return;
    }

    case WT_FLOWBOX: {
        /* Même moteur que <grid> : le nombre de colonnes est FIXÉ par
         * max-children-per-line, pas de reflux dynamique. */
        int nc = n->state.grid.columns > 0 ? n->state.grid.columns : 1;
        fprintf(out, "<div class=\"sermo-grid sermo-flowbox\" style=\"grid-template-columns:repeat(%d,auto);"
                     "row-gap:%dpx;column-gap:%dpx;\">\n",
                nc, n->state.grid.row_spacing, n->state.grid.col_spacing);
        for (int i = 0; i < n->child_count; i++) render_node(out, n->children[i], depth + 1);
        fputs("</div>\n", out);
        return;
    }

    case WT_OVERLAY:
        /* Une page SUPERPOSE vraiment : tous les enfants occupent la même
         * cellule d'une grille, le dernier au-dessus. */
        fputs("<div class=\"sermo-overlay\">\n", out);
        for (int i = 0; i < n->child_count; i++) {
            fputs("<div class=\"sermo-overlay-couche\">\n", out);
            render_node(out, n->children[i], depth + 1);
            fputs("</div>\n", out);
        }
        fputs("</div>\n", out);
        return;

    case WT_REVEALER:
        /* Caché = écrit mais masqué : la page reste complète, comme <stack>. */
        fprintf(out, "<div class=\"sermo-revealer\"%s>\n", n->state.toggle.active ? "" : " hidden");
        if (n->child_count > 0) render_node(out, n->children[0], depth + 1);
        fputs("</div>\n", out);
        return;

    case WT_TOOLBAR:
        /* <nav> : l'élément HTML qui DIT « barre d'actions » — aux lecteurs
         * d'écran comme aux feuilles de style. */
        fprintf(out, "<nav class=\"sermo-toolbar\" style=\"flex-direction:%s;gap:%dpx;\">\n",
                n->state.grid.columns == 1 ? "column" : "row", n->state.grid.col_spacing);
        for (int i = 0; i < n->child_count; i++) render_node(out, n->children[i], depth + 1);
        fputs("</nav>\n", out);
        return;

    case WT_GRID: {
        /* CSS grid : c'est exactement le même modèle que GtkGrid, colonnes
         * alignées comprises. Les écarts viennent des attributs de balise. */
        int nc = n->state.grid.columns > 0 ? n->state.grid.columns : 1;
        fprintf(out, "<div class=\"sermo-grid\" style=\"grid-template-columns:repeat(%d,auto);"
                     "row-gap:%dpx;column-gap:%dpx;\">\n",
                nc, n->state.grid.row_spacing, n->state.grid.col_spacing);
        for (int i = 0; i < n->child_count; i++) render_node(out, n->children[i], depth + 1);
        fputs("</div>\n", out);
        return;
    }

    case WT_VBOX:
        fprintf(out, "<div class=\"sermo-vbox%s\">\n", n->homogene ? " sermo-homogene" : "");
        render_enfants_boite(out, n, depth);
        fputs("</div>\n", out);
        return;

    case WT_HBOX:
        fprintf(out, "<div class=\"sermo-hbox%s\">\n", n->homogene ? " sermo-homogene" : "");
        render_enfants_boite(out, n, depth);
        fputs("</div>\n", out);
        return;

    case WT_FRAME: case WT_ASPECTFRAME:
        fputs("<fieldset class=\"sermo-frame\">", out);
        if (*lbl) { fputs("<legend class=\"sermo-legend\">", out); html_escape(out, lbl); fputs("</legend>", out); }
        fputc('\n', out);
        render_enfants_boite(out, n, depth);
        fputs("</fieldset>\n", out);
        return;

    case WT_EXPANDER:
        fputs("<details class=\"sermo-expander\"", out);
        if (n->state.expander.open) fputs(" open", out);
        fputs("><summary>", out); html_escape(out, lbl); fputs("</summary>\n", out);
        for (int i = 0; i < n->child_count; i++) render_node(out, n->children[i], depth + 1);
        fputs("</details>\n", out);
        return;

    case WT_NOTEBOOK: {
        static const char *cote[] = { "", " sermo-onglets-gauche", " sermo-onglets-droite",
                                      " sermo-onglets-bas" };
        int side = n->state.notebook.side;
        if (side < 0 || side > 3) side = 0;
        fprintf(out, "<div class=\"sermo-notebook%s\">\n", cote[side]);
        int cur = n->state.notebook.current_tab;
        if (cur < 0 || cur >= n->child_count) cur = 0;
        fputs("<div class=\"sermo-tabs\">", out);
        for (int i = 0; i < n->child_count; i++) {
            const char *tl = (n->state.notebook.tab_labels && n->state.notebook.tab_labels[i])
                             ? n->state.notebook.tab_labels[i] : "";
            fprintf(out, "<span class=\"sermo-tab%s\">", i == cur ? " sermo-tab-active" : "");
            html_escape(out, tl);
            fputs("</span>", out);
        }
        fputs("</div>\n", out);
        if (n->child_count > 0) render_node(out, n->children[cur], depth + 1);
        fputs("</div>\n", out);
        return;
    }

    case WT_MENUBAR:
        fputs("<nav class=\"sermo-menubar\"><ul>\n", out);
        for (int i = 0; i < n->child_count; i++) render_node(out, n->children[i], depth + 1);
        fputs("</ul></nav>\n", out);
        return;

    case WT_MENUITEM:
        fputs("<li class=\"sermo-item\">", out); html_escape(out, lbl); fputs("</li>\n", out);
        return;

    case WT_TREE:
        fputs("<ul class=\"sermo-tree\">\n", out);
        if (*lbl) { fputs("<li class=\"sermo-item\">", out); html_escape(out, lbl); fputs("</li>\n", out); }
        for (int i = 0; i < n->child_count; i++) render_node(out, n->children[i], depth + 1);
        fputs("</ul>\n", out);
        return;

    case WT_TIMER:
        return; /* rien de visuel : un export statique ne rejoue pas les actions périodiques */

    case WT_SEPARATOR:
        fputs("<hr class=\"sermo-separator\">\n", out);
        return;

    case WT_TEXT: case WT_STATUSBAR: {
        /* Un <text> montre ce que <input>/<default> ont posé, sinon son
         * <label>, comme l'étalon ; sans le saut de ligne final de la commande.
         * Jusqu'à la 2.8.0, seul le label : l'horloge de l'exemple restait vide. */
        const char *t = lbl;
        if (n->type == WT_TEXT && n->state.text.content && n->state.text.content[0]) t = n->state.text.content;
        size_t l = strlen(t);
        while (l > 0 && (t[l - 1] == '\n' || t[l - 1] == '\r')) l--;
        gchar *c = g_strndup(t, l);
        fprintf(out, "<p class=\"sermo-text%s\">", n->type == WT_STATUSBAR ? " sermo-statusbar" : "");
        html_escape(out, c); fputs("</p>\n", out);
        g_free(c);
        return;
    }

    case WT_INFOBAR:
        fprintf(out, "<div class=\"sermo-infobar sermo-infobar-%s\">",
                n->state.infobar.message_type[0] ? n->state.infobar.message_type : "info");
        html_escape(out, lbl);
        fputs("</div>\n", out);
        return;

    case WT_MENUBUTTON: {
        /* <details>/<summary> : un menu déroulant SANS JavaScript, que le
         * navigateur ouvre et ferme tout seul — même dans l'export statique. */
        fputs("<details class=\"sermo-menubutton\"><summary>", out);
        html_escape(out, lbl[0] ? lbl : "Menu");
        fputs("</summary>\n<ul class=\"sermo-menu\">\n", out);
        for (int i = 0; i < n->child_count; i++) {
            fputs("<li>", out);
            html_escape(out, n->children[i]->label ? n->children[i]->label : "");
            fputs("</li>\n", out);
        }
        fputs("</ul>\n</details>\n", out);
        return;
    }

    case WT_LINKBUTTON: {
        /* Le tag le plus naturel du port : un lien EST un <a href>. Le
         * navigateur ouvre, rel="noopener noreferrer" coupe l'accès à la page
         * d'origine, et un href vide devient du texte simple (pas de lien mort). */
        const char *uri = n->state.entry.buf;
        if (!uri[0]) {
            fputs("<p class=\"sermo-text\">", out); html_escape(out, lbl); fputs("</p>\n", out);
            return;
        }
        fputs("<a class=\"sermo-link\" target=\"_blank\" rel=\"noopener noreferrer\" href=\"", out);
        html_escape(out, uri);
        fputs("\">", out);
        html_escape(out, *lbl ? lbl : uri);
        fputs("</a>\n", out);
        return;
    }

    case WT_PULSE:
        /* Barre indéterminée : l'animation est en CSS (@keyframes), donc la
         * page reste autonome et anime même en export statique. */
        fputs("<div class=\"sermo-pulse\" role=\"progressbar\"", out);
        if (*lbl) { fputs(" title=\"", out); html_escape(out, lbl); fputc('"', out); }
        fputs("><span></span></div>\n", out);
        if (*lbl) { fputs("<p class=\"sermo-text\">", out); html_escape(out, lbl); fputs("</p>\n", out); }
        return;

    case WT_FILECHOOSER:
        /* Champ de CHEMIN, pas un <input type="file"> : le bac à sable du
         * navigateur ne rend qu'un nom de fichier, jamais un chemin — le
         * dialogue, lui, doit exporter un chemin utilisable par un script. */
        if (*lbl) { fputs("<label class=\"sermo-label\">", out); html_escape(out, lbl); fputs("</label>\n", out); }
        fputs("<input class=\"sermo-filechooser\" type=\"text\" placeholder=\"chemin…\" value=\"", out);
        html_escape(out, n->state.entry.buf);
        fputc('"', out);
        fld_name(out, n);
        fputs(ro(), out);
        fputs(">\n", out);
        return;

    case WT_BUTTON: case WT_TOGGLEBUTTON:
        if (html_mode() == MODE_EXPORT) {
            fputs("<button class=\"sermo-button\" type=\"button\" disabled", out);
        } else {
            /* EXIT= est la valeur que le dialogue rend pour dire quel bouton a
             * ete presse : on la porte telle quelle dans le formulaire. */
            if (n->sortie) {
                fprintf(out, "<button class=\"sermo-button\" type=\"%s\" name=\"EXIT\" value=\"",
                        html_mode() == MODE_SERVE ? "submit" : "button");
                html_escape(out, n->sortie);
                fputc('"', out);
            } else {
                /* Ses actions ne ferment pas le dialogue : il n'envoie rien. */
                fputs("<button class=\"sermo-button\" type=\"button\"", out);
            }
        }
        if (n->tooltip) { fputs(" title=\"", out); html_escape(out, n->tooltip); fputs("\"", out); }
        fputs(">", out);
        /* Bouton à icône : icône et libellé forment un groupe CENTRÉ ; l'icône
         * est placée selon image-position (gauche, droite, haut, bas). */
        if (n->icon_path) {
            static const char *pos[] = { "gauche", "droite", "haut", "bas" };
            int p = n->image_pos >= 0 && n->image_pos <= 3 ? n->image_pos : 0;
            fprintf(out, "<span class=\"sermo-btn-groupe sermo-img-%s\">", pos[p]);
            if (!html_emit_image(out, n->icon_path, "sermo-btn-icon",
                                 *lbl ? lbl : "icone", n->icon_px, n->icon_px))
                fputs("<span>\U0001F5BC️</span>", out); /* repli : absent, trop gros, ou type inconnu */
            fputs("<span>", out);
            html_escape(out, *lbl ? lbl : "Bouton");
            fputs("</span></span>", out);
        } else
            html_escape(out, *lbl ? lbl : "Bouton");
        fputs("</button>\n", out);
        return;

    case WT_CHECKBOX: case WT_SWITCH:
        /* En mode serve, une case DECOCHEE n'est pas envoyee par le navigateur :
          * le dialogue rendrait la variable absente au lieu de « false ». Un champ
          * cache homonyme pose « false » d'abord ; la case, si elle est cochee,
          * envoie « true » APRES et l'emporte (le serveur garde la derniere).
          * Inutile en mode interactive : le script lit l'etat directement. */
        if (html_mode() == MODE_SERVE && n->var_name && *n->var_name) {
            fputs("<input type=\"hidden\" name=\"", out);
            html_escape(out, n->var_name); fputs("\" value=\"false\">", out);
        }
        fprintf(out, "<label class=\"sermo-checkbox\"><input type=\"checkbox\"%s", dis());
        fld_name(out, n);
        if (html_mode() != MODE_EXPORT) fputs(" value=\"true\"", out);
        if (n->state.toggle.active) fputs(" checked", out);
        fputs("> ", out); html_escape(out, lbl); fputs("</label>\n", out);
        return;

    case WT_RADIOBUTTON:
        if (html_mode() == MODE_SERVE && n->var_name && *n->var_name) {
            fputs("<input type=\"hidden\" name=\"", out);
            html_escape(out, n->var_name); fputs("\" value=\"false\">", out);
        }
        fprintf(out, "<label class=\"sermo-radio\"><input type=\"radio\"%s", dis());
        fld_name(out, n);
        if (html_mode() != MODE_EXPORT) fputs(" value=\"true\"", out);
        if (n->state.toggle.active) fputs(" checked", out);
        fputs("> ", out); html_escape(out, lbl); fputs("</label>\n", out);
        return;

    case WT_ENTRY:
        fputs("<label class=\"sermo-field\">", out); html_escape(out, lbl);
        fprintf(out, " <input type=\"text\"%s", ro()); fld_name(out, n);
        fputs(" value=\"", out);
        html_escape(out, n->state.entry.buf); fputs("\"></label>\n", out);
        return;

    case WT_EDIT:
        /* ⛔ Le contenu d'un <edit> vit dans state.text.content, un POINTEUR de
         * tas — pas dans state.entry.buf, qui occupe la même place dans
         * l'union. Lire entry.buf écrivait les octets du pointeur dans la page
         * (mesuré : value="��jU") et lisait au-delà jusqu'au premier zéro.
         * Un <textarea> est en plus le bon élément : <edit> est multi-ligne. */
        fputs("<label class=\"sermo-field\">", out); html_escape(out, lbl);
        fputs(" <textarea rows=\"4\"", out);
        if (html_mode() == MODE_EXPORT) fputs(" readonly", out);
        fld_name(out, n);
        fputs(">", out);
        html_escape(out, n->state.text.content ? n->state.text.content : "");
        fputs("</textarea></label>\n", out);
        return;

    case WT_PASSWORD:
        fputs("<label class=\"sermo-field\">", out); html_escape(out, lbl);
        fprintf(out, " <input type=\"password\"%s", ro()); fld_name(out, n);
        fputs(" value=\"", out);
        html_escape(out, n->state.entry.buf); fputs("\"></label>\n", out);
        return;

    case WT_SEARCHENTRY:
        fputs("<label class=\"sermo-field\">", out); html_escape(out, lbl);
        fprintf(out, " <input type=\"search\"%s", ro()); fld_name(out, n);
        fputs(" value=\"", out);
        html_escape(out, n->state.list.filter); fputs("\"></label>\n", out);
        return;

    case WT_SPINBUTTON:
        fprintf(out, "<label class=\"sermo-field\">");
        html_escape(out, lbl);
        fprintf(out, " <input type=\"number\"%s", ro()); fld_name(out, n);
        fprintf(out, " value=\"%g\" min=\"%g\" max=\"%g\"></label>\n",
                n->state.spin.value, n->state.spin.min, n->state.spin.max);
        return;

    case WT_HSCALE: case WT_VSCALE:
        fprintf(out, "<label class=\"sermo-field%s\">", n->type == WT_VSCALE ? " sermo-vscale" : "");
        html_escape(out, lbl);
        fprintf(out,
                " <input type=\"range\" disabled value=\"%g\" min=\"%g\" max=\"%g\" step=\"%g\">"
                " <output>%g</output></label>\n",
                n->state.scale.value, n->state.scale.min, n->state.scale.max,
                n->state.scale.step > 0 ? n->state.scale.step : 1.0, n->state.scale.value);
        return;

    case WT_PROGRESSBAR: case WT_LEVELBAR: {
        double mx = (n->state.scale.max > n->state.scale.min) ? n->state.scale.max : 1.0;
        if (*lbl) { fputs("<label class=\"sermo-field\">", out); html_escape(out, lbl); fputs(" ", out); }
        fprintf(out, "<progress class=\"sermo-progress\" value=\"%g\" max=\"%g\"></progress>", n->state.scale.value, mx);
        if (*lbl) fputs("</label>", out);
        fputc('\n', out);
        return;
    }

    case WT_LIST: case WT_COMBOBOX: {
        /* <tree> : colonnes séparées par « | » (le cœur en fait un WT_LIST).
         * Export : un tableau sous les en-têtes du <label> ; interactif et
         * servi : la liste de choix, qui renvoie la ligne d'origine (value=)
         * mais l'affiche colonne par colonne. Jusqu'à la 2.8.0, « a|b|c » brut. */
        int arbre = 0;
        for (int i = 0; n->type == WT_LIST && i < n->state.list.item_count; i++)
            if (n->state.list.items[i] && strchr(n->state.list.items[i], '|')) arbre = 1;
        if (arbre && html_mode() == MODE_EXPORT) {
            int cols = 1;
            for (int i = 0; i < n->state.list.item_count; i++) {
                int k = 1;
                for (const char *c = n->state.list.items[i]; c && *c; c++) if (*c == '|') k++;
                if (k > cols) cols = k;
            }
            fputs("<table class=\"sermo-table sermo-tree\">\n", out);
            if (lbl[0] && strchr(lbl, '|')) {
                gchar **h = g_strsplit(lbl, "|", cols);
                fputs("<thead><tr>", out);
                for (int j = 0; j < cols; j++) {
                    fputs("<th>", out); html_escape(out, j < (int)g_strv_length(h) ? h[j] : ""); fputs("</th>", out);
                }
                fputs("</tr></thead>\n", out);
                g_strfreev(h);
            }
            fputs("<tbody>\n", out);
            for (int i = 0; i < n->state.list.item_count; i++) {
                gchar **c = g_strsplit(n->state.list.items[i] ? n->state.list.items[i] : "", "|", cols);
                fputs(i == n->state.list.selected_index ? "<tr class=\"sermo-choisi\" aria-selected=\"true\">" : "<tr>", out);
                for (int j = 0; j < cols; j++) {
                    fputs("<td>", out); html_escape(out, j < (int)g_strv_length(c) ? c[j] : ""); fputs("</td>", out);
                }
                fputs("</tr>\n", out);
                g_strfreev(c);
            }
            fputs("</tbody></table>\n", out);
            return;
        }
        if (*lbl && !arbre) { fputs("<label class=\"sermo-field\">", out); html_escape(out, lbl); fputs(" ", out); }
        fprintf(out, "<select class=\"sermo-select\"%s", dis()); fld_name(out, n);
        fprintf(out, " size=\"%d\">\n", n->type == WT_LIST ? 6 : 1);
        for (int i = 0; i < n->state.list.item_count; i++) {
            const char *it = n->state.list.items && n->state.list.items[i] ? n->state.list.items[i] : "";
            fputs("<option", out);
            if (i == n->state.list.selected_index) fputs(" selected", out);
            if (arbre) {
                gchar **c = g_strsplit(it, "|", -1);
                gchar *vu = g_strjoinv(" · ", c);
                fputs(" value=\"", out); html_escape(out, it); fputs("\">", out);
                html_escape(out, vu);
                g_free(vu); g_strfreev(c);
            } else {
                fputs(">", out);
                html_escape(out, it);
            }
            fputs("</option>\n", out);
        }
        fputs("</select>", out);
        if (*lbl && !arbre) fputs("</label>", out);
        fputc('\n', out);
        return;
    }

    case WT_CALENDAR:
        if (*lbl) { fputs("<label class=\"sermo-field\">", out); html_escape(out, lbl); fputs(" ", out); }
        fprintf(out, "<input class=\"sermo-date\" type=\"date\"%s", ro()); fld_name(out, n);
        fprintf(out, " value=\"%04d-%02d-%02d\">",
                n->state.calendar.year, n->state.calendar.month, n->state.calendar.day);
        if (*lbl) fputs("</label>", out);
        fputc('\n', out);
        return;

    case WT_COLORBUTTON:
        fputs("<label class=\"sermo-field\">", out); html_escape(out, lbl);
        fprintf(out, " <input type=\"color\"%s", dis()); fld_name(out, n);
        fprintf(out, " value=\"%s\"></label>\n",
                n->state.color.hex[0] ? n->state.color.hex : "#000000");
        return;

    case WT_FONTBUTTON:
        fputs("<label class=\"sermo-field\">", out); html_escape(out, lbl);
        fputs(" <span class=\"sermo-fontname\">", out);
        html_escape(out, n->state.font[0] ? n->state.font : "(police par défaut)");
        fputs("</span></label>\n", out);
        return;

    case WT_TABLE: {
        fputs("<table class=\"sermo-table\">\n", out);
        int cols = n->state.table.cols, rows = n->state.table.rows;
        if (n->state.table.headers && cols > 0) {
            fputs("<thead><tr>", out);
            for (int c = 0; c < cols; c++) {
                fputs("<th>", out);
                html_escape(out, n->state.table.headers[c] ? n->state.table.headers[c] : "");
                fputs("</th>", out);
            }
            fputs("</tr></thead>\n", out);
        }
        fputs("<tbody>\n", out);
        for (int r = 0; r < rows; r++) {
            fputs("<tr>", out);
            for (int c = 0; c < cols; c++) {
                fputs("<td>", out);
                if (n->state.table.cells) html_escape(out, n->state.table.cells[r * cols + c]);
                fputs("</td>", out);
            }
            fputs("</tr>\n", out);
        }
        fputs("</tbody></table>\n", out);
        return;
    }

    case WT_PIXMAP: case WT_IMAGE_W:
        /* Le chemin du fichier est range dans node->label (cf. widget_pixmap.c).
         * On l'encode en data: URI pour que la page reste autonome. Repli sur le
         * pictogramme si le fichier manque, depasse la taille maximale, ou porte
         * une extension non geree. */
        if (!html_emit_image(out, safe_label(n), "sermo-image",
                             n->tooltip ? n->tooltip : "image",
                             n->type == WT_IMAGE_W ? n->icon_px : n->width,
                             n->type == WT_IMAGE_W ? n->icon_px : n->height)) {
            /* Icône du thème introuvable : la place vide, comme gtk3 (qui ne
             * dessine rien) ; le pictogramme reste pour un FICHIER manquant. */
            if (n->type == WT_IMAGE_W && n->tooltip && n->icon_px > 0) {
                fprintf(out, "<span class=\"sermo-icone-absente\" style=\"width:%dpx;height:%dpx\"></span>\n",
                        n->icon_px, n->icon_px);
                return;
            }
            fputs("<div class=\"sermo-image-placeholder\" aria-label=\"image\">\U0001F5BC️</div>", out);
        }
        fputc('\n', out);
        return;

    case WT_DRAWINGAREA:
        fprintf(out, "<div class=\"sermo-drawingarea\" style=\"width:%gpx;height:%gpx\"></div>\n",
                n->state.drawing.width > 0 ? n->state.drawing.width : 100,
                n->state.drawing.height > 0 ? n->state.drawing.height : 100);
        return;

    case WT_SPINNER:
        fputs("<span class=\"sermo-spinner\" aria-label=\"en cours\">⟳</span>\n", out);
        return;

    case WT_TERMINAL_W:
        fputs("<pre class=\"sermo-terminal\">(sortie console non capturée dans l'export statique)</pre>\n", out);
        return;

    default:
        if (*lbl) { fputs("<div class=\"sermo-unknown\">", out); html_escape(out, lbl); fputs("</div>\n", out); }
        for (int i = 0; i < n->child_count; i++) render_node(out, n->children[i], depth + 1);
        return;
    }
}

/* ─── CSS embarqué (clair/sombre, comme les autres backends) ────────────── */
/* ─── Thème externe ───────────────────────────────────────────────────────────
 * Le rendu porte une classe `sermo-*` stable sur chaque widget : c'est le
 * CONTRAT sur lequel une feuille de style s'appuie. Deux variables :
 *
 *   SERMO_HTML_CSS=/chemin.css   embarque cette feuille APRÈS les styles
 *                                internes — la cascade fait qu'elle les
 *                                surcharge. La page reste autonome : le CSS
 *                                entre DANS la page, il n'est pas lié.
 *   SERMO_HTML_CSS_ONLY=1        n'écrit PAS les styles internes : la feuille
 *                                fournie prend tout en charge.
 *
 * Garde : une feuille contenant « </style » sortirait du bloc et injecterait
 * du HTML dans la page. On refuse le fichier entier dans ce cas plutôt que de
 * l'assainir à moitié. Idem au-delà de 256 Kio, ou si le fichier est illisible.
 */
#define CSS_MAX_BYTES (256L * 1024L)

static int write_user_css(FILE *out)
{
    const char *path = getenv("SERMO_HTML_CSS");
    if (!path || !*path) return 0;

    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "htmlsermo: feuille de style illisible : %s\n", path); return 0; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long sz = ftell(f);
    if (sz <= 0 || sz > CSS_MAX_BYTES) {
        fprintf(stderr, "htmlsermo: feuille de style ignoree (taille %ld > %ld)\n", sz, CSS_MAX_BYTES);
        fclose(f); return 0;
    }
    rewind(f);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return 0; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = '\0';

    /* Refus net : on ne laisse pas une feuille refermer le bloc <style>. */
    for (char *q = buf; (q = strchr(q, '<')); q++) {
        if (!strncmp(q, "</style", 7) || !strncmp(q, "</STYLE", 7)) {
            fprintf(stderr, "htmlsermo: feuille de style REFUSEE (contient </style>) : %s\n", path);
            free(buf); return 0;
        }
    }
    fputs("\n/* --- SERMO_HTML_CSS --- */\n", out);
    fwrite(buf, 1, got, out);
    fputc('\n', out);
    free(buf);
    return 1;
}

static void write_style(FILE *out)
{
    const char *dark_env = getenv("SERMO_DARK");
    int dark = dark_env && dark_env[0] != '0';
    const char *only = getenv("SERMO_HTML_CSS_ONLY");
    int css_only = only && only[0] != '0';
    fputs("<style>\n", out);
    if (css_only) { write_user_css(out); fputs("</style>\n", out); return; }
    fputs(
        ":root{--sermo-bg:#f4f4f5;--sermo-fg:#1c1c1e;--sermo-panel:#ffffff;"
        "--sermo-border:#c8c8cc;--sermo-accent:#3060c0;}\n", out);
    if (dark)
        fputs(
            ":root{--sermo-bg:#1c1c1e;--sermo-fg:#eaeaea;--sermo-panel:#2a2a2d;"
            "--sermo-border:#46464a;--sermo-accent:#6fa8ff;}\n", out);
    fputs(
        "body{margin:0;padding:1.5rem;background:var(--sermo-bg);color:var(--sermo-fg);"
        "font:14px/1.4 system-ui,sans-serif;}\n"
        ".sermo-window{max-width:640px;margin:0 auto;background:var(--sermo-panel);"
        "border:1px solid var(--sermo-border);border-radius:8px;padding:1.2rem;}\n"
        ".sermo-window h1{font-size:1.1rem;margin:0 0 1rem;border-bottom:1px solid var(--sermo-border);"
        "padding-bottom:.5rem;}\n"
        ".sermo-vbox{display:flex;flex-direction:column;gap:.6rem;}\n"
        ".sermo-hbox{display:flex;flex-direction:row;gap:.6rem;align-items:stretch;justify-content:flex-end;}\n"
        /* gtkdialog range une boîte horizontale PAR LA FIN (gtk_box_pack_end) :
         * ce qui ne s'étire pas se cale à droite. */
        ".sermo-hbox>.sermo-text,.sermo-hbox>.sermo-image,.sermo-hbox>.sermo-symbolique,"
        ".sermo-hbox>.sermo-icone-absente{align-self:center;}\n"
        /* Un texte GTK garde ses sauts de ligne et, dans une boîte, se centre
         * (xalign 0,5) ; la barre d'état reste à gauche. */
        ".sermo-text{margin:0;white-space:pre-wrap;}\n"
        ".sermo-vbox>.sermo-text:not(.sermo-statusbar),.sermo-etire>.sermo-text:not(.sermo-statusbar),"
        ".sermo-case>.sermo-text:not(.sermo-statusbar),.sermo-frame>.sermo-text:not(.sermo-statusbar)"
        "{text-align:center;}\n"
        ".sermo-vbox>progress,.sermo-etire>progress,.sermo-frame>progress,"
        ".sermo-vbox>meter,.sermo-etire>meter,.sermo-frame>meter{width:100%;}\n"
        ".sermo-separator{margin:.3rem 0;align-self:stretch;border:0;border-top:1px solid var(--sermo-border);}\n"
        ".sermo-icone-absente{display:inline-block;flex:none;}\n"
        /* Place d'un enfant étiré (sermo-etire) ou case d'une boîte homogène
         * (sermo-case, toutes de même taille) : orientée comme la boîte ; un
         * enfant qui remplit (space-fill) en prend toute la longueur, sinon
         * (sermo-centre) il y est centré à sa taille propre. */
        ".sermo-etire{flex:1 1 auto;min-width:0;min-height:0;display:flex;}\n"
        ".sermo-case{min-width:0;min-height:0;display:flex;}\n"
        /* Boîte homogène = grille de pistes égales : chaque case vaut la plus
         * grande taille naturelle, ou sa part de la boîte si celle-ci est
         * étirée. (En flex « 1 1 0 », une colonne non étirée s'écrasait :
         * cases de hauteur nulle, boutons superposés — vu sur system-tools.) */
        ".sermo-vbox.sermo-homogene{display:grid;grid-auto-rows:1fr;grid-template-columns:minmax(0,1fr);}\n"
        ".sermo-hbox.sermo-homogene{display:grid;grid-auto-flow:column;grid-auto-columns:minmax(0,1fr);}\n"
        ".sermo-hbox>.sermo-etire,.sermo-hbox>.sermo-case{flex-direction:row;}\n"
        ".sermo-vbox>.sermo-etire,.sermo-vbox>.sermo-case,.sermo-frame>.sermo-etire{flex-direction:column;}\n"
        ".sermo-etire>*,.sermo-case>*{flex:1 1 auto;min-width:0;}\n"
        ".sermo-centre{justify-content:center;}\n"
        ".sermo-etire.sermo-centre>*,.sermo-case.sermo-centre>*{flex:0 0 auto;}\n"
        /* Un champ, une barre, une case à cocher gardent leur hauteur propre
         * même étirés dans une colonne : centrés dans la place reçue. Un
         * <edit> (zone de texte) la remplit. */
        ":is(.sermo-vbox,.sermo-frame)>.sermo-etire>:is(progress,meter,.sermo-pulse,.sermo-checkbox,.sermo-radio,"
        ".sermo-field:not(:has(textarea))){flex:0 0 auto;margin:auto 0;}\n"
        ".sermo-etire>.sermo-field>textarea{flex:1 1 auto;}\n"
        ".sermo-hbox>:is(.sermo-etire,.sermo-case)>:is(.sermo-text,.sermo-image,.sermo-symbolique,"
        ".sermo-icone-absente){align-self:center;}\n"
        /* Une image dans une colonne est centrée sur sa largeur, comme GTK. */
        ":is(.sermo-vbox,.sermo-frame)>:is(.sermo-image,.sermo-symbolique,.sermo-icone-absente,.sermo-image-placeholder),"
        ":is(.sermo-vbox,.sermo-frame)>:is(.sermo-etire,.sermo-case)>:is(.sermo-image,.sermo-symbolique,.sermo-icone-absente)"
        "{align-self:center;}\n"
        /* L'enfant d'une fenêtre en occupe toute la hauteur (default-height). */
        ".sermo-window{display:flex;flex-direction:column;}\n"
        ".sermo-window>h1{flex:none;}\n"
        ".sermo-window>:not(h1){flex:1 1 auto;}\n"
        ".sermo-window{overflow-x:auto;}\n"
        ".sermo-notebook.sermo-onglets-gauche,.sermo-notebook.sermo-onglets-droite{display:flex;gap:.6rem;}\n"
        ".sermo-onglets-droite{flex-direction:row-reverse;}\n"
        ".sermo-onglets-bas{display:flex;flex-direction:column-reverse;}\n"
        ".sermo-onglets-gauche>.sermo-tabs,.sermo-onglets-droite>.sermo-tabs{flex-direction:column;"
        "border-bottom:0;border-right:1px solid var(--sermo-border);margin:0;padding-right:.4rem;}\n"
        ".sermo-onglets-gauche>.sermo-tabs .sermo-tab-active,.sermo-onglets-droite>.sermo-tabs .sermo-tab-active"
        "{border-bottom:0;border-left:2px solid var(--sermo-accent);}\n"
        ".sermo-onglets-gauche>:not(.sermo-tabs),.sermo-onglets-droite>:not(.sermo-tabs){flex:1 1 auto;min-width:0;}\n"
        ".sermo-frame{border:1px solid var(--sermo-border);border-radius:6px;padding:.7rem;"
        "display:flex;flex-direction:column;gap:.6rem;min-width:0;}\n"
        ".sermo-field{display:flex;flex-direction:column;gap:.2rem;font-size:.85rem;}\n"
        ".sermo-field input,.sermo-field select{font:inherit;}\n"
        "button,input,select,progress{accent-color:var(--sermo-accent);}\n"
        "button{border:1px solid var(--sermo-border);border-radius:4px;padding:.3rem .8rem;"
        "background:var(--sermo-panel);color:var(--sermo-fg);}\n"
        ".sermo-tabs{display:flex;gap:.4rem;border-bottom:1px solid var(--sermo-border);"
        "margin-bottom:.6rem;}\n"
        ".sermo-tab{padding:.2rem .6rem;opacity:.6;}\n"
        ".sermo-tab-active{opacity:1;border-bottom:2px solid var(--sermo-accent);}\n"
        ".sermo-infobar{padding:.4rem .7rem;border-radius:4px;background:var(--sermo-border);}\n"
        /* teinte selon message-type, texte noir : contraste >= 12:1 */
        ".sermo-infobar-info{background:#e3eef9;color:#000;}"
        ".sermo-infobar-warning{background:#f4e7d3;color:#000;}"
        ".sermo-infobar-question{background:#dce6f4;color:#000;}"
        ".sermo-infobar-error{background:#f6d5d5;color:#000;}\n"
        ".sermo-tree{width:100%;}\n"
        ".sermo-tree th{text-align:left;font-weight:normal;opacity:.75;}\n"
        ".sermo-tree .sermo-choisi td{background:#1a3a5c;color:#fff;}\n"
        ".sermo-statusbar{border-top:1px solid var(--sermo-border);padding-top:.3rem;}\n"
        ".sermo-vscale input[type=range]{writing-mode:vertical-lr;direction:rtl;height:8rem;width:auto;}\n"
        ".sermo-table{border-collapse:collapse;}\n"
        ".sermo-table th,.sermo-table td{border:1px solid var(--sermo-border);padding:.3rem .5rem;}\n"
        ".sermo-image{max-width:100%;height:auto;display:block;}"
        ".sermo-btn-icon{height:1em;width:auto;vertical-align:-0.15em;}"
        ".sermo-btn-groupe{display:inline-flex;align-items:center;justify-content:center;gap:.35em;"
        "vertical-align:middle;}"
        ".sermo-img-droite{flex-direction:row-reverse;}.sermo-img-haut{flex-direction:column;}"
        ".sermo-img-bas{flex-direction:column-reverse;}"
        ".sermo-symbolique{display:inline-block;background:currentColor;flex:none;"
        "-webkit-mask-size:contain;mask-size:contain;-webkit-mask-repeat:no-repeat;mask-repeat:no-repeat;"
        "-webkit-mask-position:center;mask-position:center;}"
        ".sermo-image{flex:none;}"
        ".sermo-image-placeholder{width:2rem;height:2rem;display:flex;align-items:center;"
        "justify-content:center;border:1px dashed var(--sermo-border);border-radius:4px;}\n"
        ".sermo-eventbox{display:block;}\n"
        ".sermo-grid{display:grid;align-items:center;}\n"
        ".sermo-stack-page[hidden]{display:none;}\n"
        ".sermo-revealer[hidden]{display:none;}\n"
        ".sermo-overlay{display:grid;}\n"
        ".sermo-overlay-couche{grid-area:1/1;}\n"
        ".sermo-menubutton>summary{cursor:pointer;display:inline-block;"
        "padding:.2rem .6rem;border:1px solid var(--sermo-border);border-radius:4px;}\n"
        ".sermo-menu{list-style:none;margin:.2rem 0;padding:.2rem;"
        "border:1px solid var(--sermo-border);border-radius:4px;}\n"
        ".sermo-toolbar{display:flex;align-items:center;padding:.3rem;"
        "border-bottom:1px solid var(--sermo-border);margin-bottom:.4rem;}\n"
        ".sermo-paned{display:flex;gap:0;align-items:stretch;}\n"
        ".sermo-paned-part{overflow:auto;min-width:2rem;min-height:2rem;}\n"
        ".sermo-paned-poignee{flex:0 0 4px;background:var(--sermo-border);cursor:col-resize;}\n"
        ".sermo-link{color:var(--sermo-accent);text-decoration:underline;}\n"
        ".sermo-filechooser{width:100%;box-sizing:border-box;}\n"
        ".sermo-pulse{position:relative;height:.75rem;border-radius:.375rem;"
        "background:var(--sermo-border);overflow:hidden;margin:.25rem 0;}\n"
        ".sermo-pulse>span{position:absolute;top:0;bottom:0;width:30%;"
        "border-radius:.375rem;background:var(--sermo-accent);"
        "animation:sermo-va-et-vient 1.6s ease-in-out infinite alternate;}\n"
        "@keyframes sermo-va-et-vient{from{left:0;}to{left:70%;}}\n"
        /* Respecte « animations réduites » : la barre se fige au lieu de
         * bouger sans fin (accessibilité, et sobriété sur batterie). */
        "@media (prefers-reduced-motion:reduce){.sermo-pulse>span{animation:none;left:0;}}\n"
        ".sermo-note{max-width:640px;margin:0 auto 1rem;font-size:.75rem;opacity:.7;}\n",
        out);
    write_user_css(out);
    fputs("</style>\n", out);
}

/* ─── Mode interactif : la page se répond à elle-même ─────────────────────────
 * Aucun réseau. Le script lit le formulaire, compose le bloc VAR="valeur" que
 * le dialogue aurait imprimé, et propose de le copier ou de le télécharger.
 * Les valeurs sont échappées côté script comme le cœur le fait côté C
 * (antislash, guillemet, dollar, accent grave) pour qu'un collage dans un
 * shell ne puisse pas exécuter ce que l'utilisateur a tapé. */
static void write_interactive_js(FILE *out)
{
    fputs(
"<div class=\"sermo-result\" hidden id=\"sermo-out\">\n"
"<pre id=\"sermo-pre\"></pre>\n"
"<button type=\"button\" id=\"sermo-copy\">Copier</button>\n"
"<a id=\"sermo-dl\" download=\"sermo-valeurs.txt\">Télécharger</a>\n"
"</div>\n"
"<p><button type=\"button\" id=\"sermo-go\">Composer le résultat</button></p>\n"
"<script>\n"
"(function(){\n"
"  var esc=function(v){return String(v).replace(/[\\\\\"$`]/g,function(c){return \"\\\\\"+c;});};\n"
"  var f=document.getElementById('sermo-form');\n"
"  var pre=document.getElementById('sermo-pre');\n"
"  var box=document.getElementById('sermo-out');\n"
"  var exitv='';\n"
"  Array.prototype.forEach.call(f.querySelectorAll('button[name=EXIT]'),function(b){\n"
"    b.addEventListener('click',function(){exitv=b.value;compose();});\n"
"  });\n"
"  function compose(){\n"
"    var seen={},lines=[];\n"
"    Array.prototype.forEach.call(f.elements,function(el){\n"
"      if(!el.name||el.name==='EXIT'||el.type==='hidden')return;\n"
"      if(el.type==='checkbox'||el.type==='radio'){\n"
"        if(el.type==='radio'&&!el.checked)return;\n"
"        if(seen[el.name])return; seen[el.name]=1;\n"
"        lines.push(el.name+'=\"'+(el.checked?'true':'false')+'\"');\n"
"      }else{\n"
"        if(seen[el.name])return; seen[el.name]=1;\n"
"        lines.push(el.name+'=\"'+esc(el.value)+'\"');\n"
"      }\n"
"    });\n"
"    if(exitv)lines.push('EXIT=\"'+esc(exitv)+'\"');\n"
"    var txt=lines.join('\\n')+'\\n';\n"
"    pre.textContent=txt; box.hidden=false;\n"
"    var dl=document.getElementById('sermo-dl');\n"
"    dl.href='data:text/plain;charset=utf-8,'+encodeURIComponent(txt);\n"
"  }\n"
"  document.getElementById('sermo-go').addEventListener('click',compose);\n"
"  document.getElementById('sermo-copy').addEventListener('click',function(){\n"
"    if(navigator.clipboard)navigator.clipboard.writeText(pre.textContent);\n"
"  });\n"
"})();\n"
"</script>\n", out);
}

/* Écrit la page dans `out`. Séparé de render_html_export pour que le mode
 * « serve » puisse produire la même page dans un flux mémoire (open_memstream)
 * au lieu d'un fichier — une seule source de vérité pour les trois modes. */
void render_html_write(FILE *out, WidgetNode *root, const char *title)
{
    fputs("<!doctype html>\n<html lang=\"fr\">\n<head>\n<meta charset=\"utf-8\">\n", out);
    fputs("<title>", out);
    html_escape(out, (title && *title) ? title : "sermo");
    fputs("</title>\n", out);
    write_style(out);
    fputs("</head>\n<body>\n", out);

    switch (html_mode()) {
    case MODE_EXPORT:
        fputs("<p class=\"sermo-note\">Export statique généré par htmlsermo : "
              "page morte, les champs ne se saisissent pas.</p>\n", out);
        render_node(out, root, 0);
        break;

    case MODE_INTERACTIVE:
        fputs("<p class=\"sermo-note\">Page autonome : remplissez, puis "
              "« Composer le résultat ». Rien n'est envoyé nulle part.</p>\n", out);
        fputs("<form id=\"sermo-form\" onsubmit=\"return false\">\n", out);
        render_node(out, root, 0);
        fputs("</form>\n", out);
        write_interactive_js(out);
        break;

    case MODE_SERVE:
        fputs("<form id=\"sermo-form\" method=\"post\" action=\"\">\n", out);
        render_node(out, root, 0);
        fputs("</form>\n", out);
        break;
    }

    fputs("</body>\n</html>\n", out);
}

int render_html_export(WidgetNode *root, const char *title)
{
    if (!root) return 0;

    const char *out_path = getenv("SERMO_HTML_OUT");
    if (!out_path || !*out_path) out_path = "sermo-output.html";

    FILE *out = (strcmp(out_path, "-") == 0) ? stdout : fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "htmlsermo: impossible d'écrire %s\n", out_path);
        return 1;
    }
    render_html_write(out, root, title);
    if (out != stdout) fclose(out);
    return 0;
}
