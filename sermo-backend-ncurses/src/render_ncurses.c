/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* render_ncurses.c — renderer + boucle d'évènements du backend ncurses.
 *
 * Reprend le modèle « arbre de WidgetNode » toolkit-agnostique du port neutre
 * (sdl3), rendu dans un TERMINAL via ncurses. Toute la logique (parse,
 * variables, actions, timers) vit dans le cœur ; ce fichier (a) dispose et
 * dessine l'arbre et (b) route le clavier vers execute_action() du cœur.
 *
 * Deux chemins :
 *  - HEADLESS (entrée standard qui n'est pas un terminal, ou
 *    SERMO_NCURSES_BATCH=1) : n'ouvre pas ncurses ; exécute la logique
 *    (timers → actions ; un « exit:… » fait exporter et sortir le cœur).
 *    Sert aux bancs et aux lancements sans personne au clavier (cron, tube).
 *  - INTERACTIF (quelqu'un au clavier) : moteur de disposition récursif (vbox
 *    empile, hbox aligne, frame encadre, notebook onglets) + rendu riche des
 *    widgets, navigation Tab/flèches, activation Entrée/Espace.
 *    L'écran va sur la sortie standard si c'est un terminal, sinon sur le
 *    terminal de contrôle (/dev/tty) : `eval "$(ncursessermo …)"` capture la
 *    sortie, et c'est justement là qu'un dialogue doit s'ouvrir. Les variables,
 *    elles, partent toujours sur la sortie standard.
 */
#include <ncurses.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <strings.h>   /* strcasecmp (thème) */
#include <unistd.h>

#include "render_ncurses.h"
#include "dialog_state.h"
#include "variables.h"
#include "actions.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "sermo_open_uri.h"
#include "sermo_progress.h"
#include "jouer_actions.h"

/* Fourni par le cœur (actions.c), non exporté par ses en-têtes — comme dans sdl3. */
void action_exitprogram(GtkWidget *widget, char *string);

/* ═══════════════════════════════════════════════════════════════════════════
 *  Déclenchement des <timer> + export (partagé headless/interactif)
 * ═══════════════════════════════════════════════════════════════════════════ */
/* Joue les actions d'un nœud : toutes, chacune avec sa fonction, quand le
 * créateur a gardé le jeu d'attributs ; sinon l'unique commande du nœud. */
static void declencher(WidgetNode *n, const char *signal1, const char *signal2)
{
    if (!n) return;
    if (n->actions_attr)
        jouer_actions((GtkWidget *)n, (AttributeSet *)n->actions_attr, signal1, signal2);
    else if (n->action && *n->action)
        execute_action((GtkWidget *)n, n->action, NULL);
}

/* Widgets à bascule (case à cocher, bouton radio, bouton à bascule,
 * interrupteur) : leurs actions jouent à chaque changement d'état, « if true »/
 * « if false » compris. */
static void bascule_jouer(WidgetNode *n, int actif)
{
    if (n->actions_attr && n->type == WT_SWITCH)
        jouer_actions_interrupteur((GtkWidget *)n, (AttributeSet *)n->actions_attr, actif);
    else if (n->actions_attr)
        jouer_actions_bascule((GtkWidget *)n, (AttributeSet *)n->actions_attr, actif);
    else if (n->action && *n->action)
        execute_action((GtkWidget *)n, n->action, NULL);
}

/* Pour widget_radiobutton.c : le bouton radio qu'un autre remplace au départ
 * (son <default> ou son <input>) joue ses actions « décoché », comme chez
 * l'étalon, où elles sont déjà branchées. */
void sermo_be_bascule_jouer(void *w, int actif)
{
    if (w) bascule_jouer((WidgetNode *)w, actif);
}

/* Bascule comme un clic de l'étalon. Un bouton radio choisi le reste ; en
 * choisir un décoche ses frères de la même boîte (le groupe du cœur), qui
 * jouent leurs actions « décoché » d'abord, comme GTK. */
static void basculer(WidgetNode *n)
{
    if (n->type == WT_RADIOBUTTON) {
        if (n->state.toggle.active) return;
        n->state.toggle.active = TRUE;
        if (n->parent)
            for (int i = 0; i < n->parent->child_count; i++) {
                WidgetNode *f = n->parent->children[i];
                if (f == n || f->type != WT_RADIOBUTTON || !f->state.toggle.active) continue;
                f->state.toggle.active = FALSE;
                bascule_jouer(f, FALSE);
            }
        bascule_jouer(n, TRUE);
        return;
    }
    n->state.toggle.active = !n->state.toggle.active;
    bascule_jouer(n, n->state.toggle.active);
}

/* Crochet du contrat : « activate:NOM » bascule les widgets à bascule (le shim
 * fait de gtk_widget_activate() un FALSE). Le prochain tour redessine. */
int sermo_be_widget_activate(void *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return 0;
    switch (n->type) {
    case WT_CHECKBOX: case WT_RADIOBUTTON: case WT_TOGGLEBUTTON: case WT_SWITCH:
        basculer(n);
        return 1;
    default:
        return 0;
    }
}

/* Crochet du contrat : l'état d'un widget pour condition="active_is_…/
 * sensitive_is_…/visible_is_…(NOM)" (le shim ne sait rien des nœuds).
 * quoi : 0 = actif, 1 = sensible, 2 = visible ; -1 = je ne sais pas. */
int sermo_be_widget_etat(void *w, int quoi)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return -1;
    switch (quoi) {
    case 0:
        switch (n->type) {
        case WT_CHECKBOX: case WT_RADIOBUTTON: case WT_TOGGLEBUTTON: case WT_SWITCH:
            return n->state.toggle.active ? 1 : 0;
        case WT_EXPANDER:
            return n->state.expander.open ? 1 : 0;
        default:
            return -1;
        }
    case 1: return n->sensitive ? 1 : 0;
    case 2: return n->visible ? 1 : 0;
    default: return -1;
    }
}

/* <list> : joue ses actions (signal « selection-changed » de l'étalon). Seule
 * une vraie <list> garde son jeu d'actions ; un <tree> (WT_LIST aussi) non. */
static void liste_selection_changee(WidgetNode *n)
{
    if (n && n->type == WT_LIST && n->actions_attr)
        declencher(n, "selection-changed", "row-selected");
}

/* La sélection posée au chargement est jouée une fois, l'arbre entier
 * construit : les variables que l'action vise existent alors. Avec ou sans
 * écran, avant toute minuterie. */
static void listes_chargees(WidgetNode *n)
{
    if (!n) return;
    if (n->type == WT_LIST && n->state.list.selected_index >= 0)
        liste_selection_changee(n);
    for (int i = 0; i < n->child_count; i++) listes_chargees(n->children[i]);
}

/* Minuteries. label = intervalle en millisecondes (widget_timer.c) ;
 * state.scale.value = prochaine échéance, en secondes d'horloge monotone.
 * Jusqu'à la 2.8.1, chaque minuterie partait à CHAQUE tour de boucle (en
 * terminal) ou toutes dans l'ordre du document (sans terminal) : une
 * minuterie de 8 s fermait le dialogue avant celle d'une seconde. */
static double maintenant(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

static double intervalle(const WidgetNode *n)
{
    int ms = (n->label && *n->label) ? atoi(n->label) : 1000;
    return (ms > 0 ? ms : 1000) / 1000.0;
}

/* En terminal : chaque minuterie part à son échéance, puis repart. */
static void fire_timers(WidgetNode *node)
{
    if (!node) return;
    if (node->type == WT_TIMER && node->sensitive) {
        double t = maintenant(), iv = intervalle(node);
        if (node->state.scale.value <= 0)
            node->state.scale.value = t + iv;
        else if (t >= node->state.scale.value) {
            node->state.scale.value = t + iv;
            declencher(node, "tick", NULL);
        }
    }
    for (int i = 0; i < node->child_count; i++) fire_timers(node->children[i]);
}

/* Sans terminal : chacune une fois, de la plus courte à la plus longue —
 * l'ordre où elles partiraient à l'écran (un « exit:… » sort au passage). */
static void lister_timers(WidgetNode *n, WidgetNode ***t, int *nb, int *cap)
{
    if (!n) return;
    if (n->type == WT_TIMER && n->sensitive) {
        if (*nb == *cap) { *cap = *cap ? *cap * 2 : 8; *t = realloc(*t, (size_t)*cap * sizeof **t); }
        if (*t) (*t)[(*nb)++] = n;
    }
    for (int i = 0; i < n->child_count; i++) lister_timers(n->children[i], t, nb, cap);
}
static int par_intervalle(const void *a, const void *b)
{
    double x = intervalle(*(WidgetNode *const *)a), y = intervalle(*(WidgetNode *const *)b);
    return (x > y) - (x < y);
}
static void fire_timers_une_fois(WidgetNode *root)
{
    WidgetNode **t = NULL;
    int nb = 0, cap = 0;
    lister_timers(root, &t, &nb, &cap);
    /* tri par insertion : STABLE, deux délais égaux gardent l'ordre du document */
    for (int i = 1; i < nb; i++)
        for (int j = i; j > 0 && par_intervalle(&t[j - 1], &t[j]) > 0; j--) {
            WidgetNode *x = t[j]; t[j] = t[j - 1]; t[j - 1] = x;
        }
    for (int i = 0; i < nb; i++) declencher(t[i], "tick", NULL);
    free(t);
}

/* Barres de progression : relever ce que leurs commandes ont écrit.
 * Rend le nombre de lectures encore ouvertes. */
static int servir_barres(WidgetNode *node)
{
    int ouvertes = 0;
    if (!node) return 0;
    if (node->type == WT_PROGRESSBAR && node->lecture) {
        if (sermo_progress_poll((sermo_progress *)node->lecture))
            ouvertes++;
        else
            node->lecture = NULL;   /* rendue par le cœur */
    }
    for (int i = 0; i < node->child_count; i++) ouvertes += servir_barres(node->children[i]);
    return ouvertes;
}

static int export_abort(void)
{
    variables_export_all();
    printf("EXIT=\"abort\"\n");
    g_setenv("EXIT", "abort", TRUE);   /* --do lit EXIT dans l'environnement (manuel §3.2) : il n'y était pas. */
    fflush(stdout);
    return 0;
}

static int run_headless(WidgetNode *root)
{
    /* Sans écran, rien ne presse : chaque barre lit sa commande jusqu'au bout
     * (la première ligne à 100 déclenche ses actions, un « exit:… » sort ici),
     * puis les minuteries partent une fois. Une commande sans fin bloque, comme
     * la lecture d'un seul tenant le faisait déjà jusqu'à la 2.6.8. */
    /* nanosleep, pas usleep : retiré de POSIX 2008, que ce port demande
     * (_POSIX_C_SOURCE 200809L) ; FreeBSD le cache alors. */
    const struct timespec pause = { 0, 20000000L };   /* 20 ms */
    while (servir_barres(root) > 0)
        nanosleep(&pause, NULL);
    fire_timers_une_fois(root);   /* un « exit:… » sort ici via action_exitprogram */
    return export_abort();
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Moteur de disposition + rendu (interactif)
 * ═══════════════════════════════════════════════════════════════════════════ */
typedef struct { WidgetNode *n; int y, x, w; } Focusable;
static Focusable g_focus[256];
static int       g_nfocus;
static int       g_focus_idx;

static int node_focusable(WidgetType t)
{
    switch (t) {
    case WT_BUTTON: case WT_ENTRY: case WT_EDIT: case WT_PASSWORD:
    case WT_CHECKBOX: case WT_RADIOBUTTON: case WT_TOGGLEBUTTON:
    case WT_SWITCH: case WT_LIST: case WT_COMBOBOX: case WT_SPINBUTTON:
    case WT_HSCALE: case WT_VSCALE: case WT_SEARCHENTRY:
    case WT_LINKBUTTON: case WT_FILECHOOSER: return 1;
    case WT_PANED: case WT_WIZARD: case WT_MENUBUTTON: return 1;
    case WT_EXPANDER: return 1;
    default: return 0;
    }
}

/* Barre de progression / échelle : "[####----]  42%" (frac 0..1). */
static void draw_bar(int y, int x, int w, double frac, char *out, int outsz)
{
    if (frac < 0) frac = 0; if (frac > 1) frac = 1;
    int inner = w > 4 ? w - 2 : 2;
    int fill = (int)(frac * inner + 0.5);
    char bar[128]; int k = inner < 120 ? inner : 120;
    for (int i = 0; i < k; i++) bar[i] = (i < fill) ? '#' : '-';
    bar[k] = '\0';
    snprintf(out, outsz, "[%s]", bar);
    (void)y; (void)x;
}

/* <tree> (un WT_LIST du cœur) : colonnes « a|b|c » alignées, chaque colonne à
 * la largeur de sa plus longue cellule (en caractères). Jusqu'à la 2.8.0, la
 * ligne brute « a|b|c » s'affichait. Rend le nombre de colonnes (1 = liste). */
static int colonnes_mesurer(const WidgetNode *n, int *larg, int max)
{
    int nc = 1;
    for (int i = -1; i < n->state.list.item_count; i++) {
        const char *t = i < 0 ? n->label : (n->state.list.items ? n->state.list.items[i] : NULL);
        if (!t || !strchr(t, '|')) continue;
        gchar **c = g_strsplit(t, "|", max);
        for (int j = 0; c[j] && j < max; j++) {
            int l = (int)g_utf8_strlen(c[j], -1);
            if (j >= nc) { larg[j] = 0; nc = j + 1; }
            if (l > larg[j]) larg[j] = l;
        }
        g_strfreev(c);
    }
    return nc;
}
static void colonnes_ecrire(const char *t, const int *larg, int nc, char *out, size_t sz)
{
    gchar **c = g_strsplit(t ? t : "", "|", nc);
    size_t o = 0;
    out[0] = '\0';
    for (int j = 0; j < nc && o + 1 < sz; j++) {
        const char *v = (j < (int)g_strv_length(c)) ? c[j] : "";
        int l = (int)g_utf8_strlen(v, -1);
        o += (size_t)snprintf(out + o, sz - o, "%s%*s", v, (j < nc - 1) ? larg[j] - l + 2 : 0, "");
        if (o >= sz) { out[sz - 1] = '\0'; break; }
    }
    g_strfreev(c);
}

/* Texte d'une feuille sur une ligne (sans le dessiner) : sert au dessin ET à la
 * mesure de la largeur naturelle (rangée). etirer = la rangée lui donne une
 * case qu'il doit remplir (space-fill) : bouton et champs s'élargissent. */
static void ligne_feuille(WidgetNode *n, int w, int etirer, char *line, size_t sz)
{
    const char *lbl = n->label ? n->label : "";
    /* largeur d'un champ : 16 colonnes, ou toute la case qu'il remplit */
    int champ = 16;
    if (etirer) {
        int reste = w - (int)g_utf8_strlen(lbl, -1) - 2;
        if (reste > champ) champ = reste;
    }
    switch (n->type) {
    case WT_TEXT: case WT_STATUSBAR: {
        /* <text> : ce que <input>/<default> ont posé, sinon le <label> ;
         * sans le saut de ligne final de la commande. */
        const char *t = lbl;
        if (n->type == WT_TEXT && n->state.text.content && n->state.text.content[0]) t = n->state.text.content;
        snprintf(line, sz, "%s", t);
        size_t l = strlen(line);
        while (l > 0 && (line[l - 1] == '\n' || line[l - 1] == '\r')) line[--l] = '\0';
        break;
    }
    case WT_BUTTON: {
        /* Bouton qui remplit sa case : les crochets aux bords, le libellé au
         * milieu (en terminal, le bouton n'a pas d'icône : point 4 sans objet). */
        const char *t = lbl[0] ? lbl : "OK";
        int lw = (int)g_utf8_strlen(t, -1), dedans = w - 2;
        if (etirer && dedans > lw + 2) {
            int g = (dedans - lw) / 2;
            snprintf(line, sz, "[%*s%s%*s]", g, "", t, dedans - lw - g, "");
        } else {
            snprintf(line, sz, "[ %s ]", t);
        }
        break;
    }
    case WT_CHECKBOX: case WT_TOGGLEBUTTON: case WT_SWITCH:
        snprintf(line, sz, "[%c] %s", n->state.toggle.active ? 'x' : ' ', lbl); break;
    case WT_RADIOBUTTON:
        snprintf(line, sz, "(%c) %s", n->state.toggle.active ? '*' : ' ', lbl); break;
    case WT_ENTRY:
        snprintf(line, sz, "%s[%-*s]", lbl[0] ? lbl : "", champ, n->state.entry.buf); break;
    case WT_EDIT: {
        /* ⛔ Le contenu d'un <edit> vit dans state.text.content, un
         * POINTEUR de tas — pas dans state.entry.buf. Les deux occupent la
         * même place dans l'union : lire entry.buf ici revenait à afficher
         * les octets du pointeur, puis à lire au-delà jusqu'au premier
         * zéro (débordement de lecture). Mesuré : « ��jU » à l'écran. */
        const char *contenu = n->state.text.content ? n->state.text.content : "";
        snprintf(line, sz, "%s[%-*s]", lbl[0] ? lbl : "", champ, contenu);
        break;
    }
    case WT_SEARCHENTRY:
        snprintf(line, sz, "%s[%-16s]?", lbl[0] ? lbl : "", n->state.list.filter); break;
    case WT_PASSWORD: {
        int L = (int)strlen(n->state.entry.buf), k = L < 16 ? L : 16;
        char st[24]; memset(st, '*', k); st[k] = '\0';
        snprintf(line, sz, "%s[%-*s]", lbl[0] ? lbl : "", champ, st); break;
    }
    case WT_SPINBUTTON:
        snprintf(line, sz, "%s[%d]", lbl[0] ? lbl : "", (int)n->state.spin.value); break;
    case WT_PROGRESSBAR: case WT_LEVELBAR: {
        char bar[160];
        double mx = (n->state.scale.max > n->state.scale.min) ? n->state.scale.max : 1.0;
        draw_bar(0, 0, w > 30 ? 30 : w, n->state.scale.value / (mx ? mx : 1.0), bar, sizeof bar);
        snprintf(line, sz, "%s%s", lbl[0] ? lbl : "", bar); break;
    }
    case WT_HSCALE: case WT_VSCALE: {
        char bar[160];
        double lo = n->state.scale.min, hi = n->state.scale.max, v = n->state.scale.value;
        double frac = (hi > lo) ? (v - lo) / (hi - lo) : 0;
        draw_bar(0, 0, w > 24 ? 24 : w, frac, bar, sizeof bar);
        snprintf(line, sz, "%s%s %.6g", lbl[0] ? lbl : "", bar, v); break;
    }
    case WT_COMBOBOX: {
        int si = n->state.list.selected_index;
        const char *it = (si >= 0 && si < n->state.list.item_count &&
                          n->state.list.items && n->state.list.items[si])
                         ? n->state.list.items[si] : "";
        snprintf(line, sz, "%s< %s >", lbl[0] ? lbl : "", it);
        break;
    }
    case WT_INFOBAR:
        snprintf(line, sz, "[%s] %s", n->state.infobar.message_type, lbl); break;
    case WT_MENUBUTTON:
        /* Le libellé, puis le choix courant entre chevrons — en terminal
         * un menu déroulant se lit mieux ainsi qu'il ne se déroule. */
        snprintf(line, sz, "[ %s \u25be ] %s", lbl[0] ? lbl : "Menu",
                 n->state.entry.buf);
        break;
    case WT_LINKBUTTON:
        /* Le libellé, puis l'URI entre chevrons — en terminal le lien ne
         * se « survole » pas : l'adresse doit être lisible telle quelle. */
        if (n->state.entry.buf[0] && strcmp(lbl, n->state.entry.buf) != 0)
            snprintf(line, sz, "%s <%s>", lbl, n->state.entry.buf);
        else
            snprintf(line, sz, "<%s>", n->state.entry.buf);
        break;
    case WT_FILECHOOSER:
        snprintf(line, sz, "%s[%-24s]", lbl[0] ? lbl : "",
                 n->state.entry.buf[0] ? n->state.entry.buf : "(choisir…)");
        break;
    case WT_PULSE: {
        /* Bloc qui va et vient : « ça travaille », sans valeur. */
        int inner = (w > 32 ? 30 : (w > 6 ? w - 2 : 4));
        int blk   = inner / 5 > 2 ? inner / 5 : 2;
        int span  = inner - blk;
        int phase = (int) n->state.spinner.angle;
        int pos   = span > 0 ? phase % (2 * span) : 0;
        if (pos > span) pos = 2 * span - pos;       /* aller-retour */
        char bar[128];
        int k = inner < 120 ? inner : 120;
        for (int i = 0; i < k; i++) bar[i] = (i >= pos && i < pos + blk) ? '#' : '-';
        bar[k] = '\0';
        snprintf(line, sz, "%s[%s]", lbl[0] ? lbl : "", bar);
        break;
    }
    case WT_CALENDAR:
        snprintf(line, sz, "%s<%04d-%02d-%02d>", lbl[0] ? lbl : "",
                 n->state.calendar.year, n->state.calendar.month, n->state.calendar.day); break;
    default:
        snprintf(line, sz, "%s", lbl); break;
    }
}

/* Écart entre deux voisins d'une rangée : une colonne (gtk3 : 5 px). */
#define ESPACE_RANGEE 1

/* Le réglage d'étirement que la rangée passe au prochain draw_node (lu et remis
 * à zéro à son entrée : il ne vaut que pour l'enfant visé, pas pour ses
 * descendants). */
static int g_etirer;

/* Hauteur imposée (en lignes) au prochain draw_node, même règle de passage :
 * une colonne étirée, ou la fenêtre qui a une hauteur, la donne à l'enfant
 * qui la remplit. 0 = hauteur naturelle. */
static int g_haut;

/* Une ligne de terminal vaut à peu près 16 px de l'étalon (police de 12). */
#define PX_PAR_LIGNE 16

/* Efface un rectangle (avant de redessiner une colonne répartie). */
static void effacer_zone(int y, int x, int h, int w)
{
    for (int i = 0; i < h; i++) mvprintw(y + i, x, "%*s", w > 0 ? w : 0, "");
}

/* Largeur NATURELLE d'un nœud, en colonnes : ce qu'il occupe quand on ne
 * l'étire pas. Sert à la rangée (<hbox>) pour caler à droite et partager le
 * reste ; 0 = ne prend aucune place (image, minuterie, caché). */
static int largeur(WidgetNode *n)
{
    if (!n || !n->visible) return 0;
    const char *lbl = n->label ? n->label : "";
    int lw = (int)g_utf8_strlen(lbl, -1);
    int m = 0, s = 0, k = 0;
    for (int i = 0; i < n->child_count; i++) {
        int l = largeur(n->children[i]);
        if (l > 0) { s += l; k++; if (l > m) m = l; }
    }
    switch (n->type) {
    case WT_TIMER: case WT_IMAGE_W: case WT_PIXMAP:
        return 0;
    case WT_SEPARATOR:
        return 1;
    case WT_HBOX:
        return k ? (n->homogeneous ? m * k : s) + (k - 1) * ESPACE_RANGEE : 0;
    case WT_VBOX: case WT_EVENTBOX: case WT_WINDOW:
        return m;
    case WT_REVEALER:
        return (n->state.toggle.active && n->child_count) ? largeur(n->children[0]) : 0;
    case WT_FRAME: case WT_ASPECTFRAME:
        return (m > lw + 4 ? m : lw + 4) + 2;
    case WT_EXPANDER:
        return lw + 4 > m + 2 ? lw + 4 : m + 2;
    case WT_WIZARD:
        return m > 40 ? m : 40;
    case WT_OVERLAY:
        return m > 13 ? m : 13;
    case WT_STACK:
        return m > 4 * n->child_count ? m : 4 * n->child_count;
    case WT_NOTEBOOK: {
        int t = 0;
        for (int i = 0; i < n->child_count; i++) {
            const char *tl = (n->state.notebook.tab_labels && n->state.notebook.tab_labels[i])
                             ? n->state.notebook.tab_labels[i] : "•";
            t += (int)g_utf8_strlen(tl, -1) + 3;
        }
        return t > m ? t : m;
    }
    case WT_PANED:
        return n->state.paned.vertical ? m : s + 1 > 9 ? s + 1 : 9;
    case WT_TOOLBAR:
        if (n->state.grid.columns == 1) return m;
        return n->child_count * (m + 1);
    case WT_GRID: case WT_FLOWBOX: {
        int nc = n->state.grid.columns > 0 ? n->state.grid.columns : 1;
        return nc * (m + 1);
    }
    case WT_LIST: {
        int larg[16] = {0}, nc = colonnes_mesurer(n, larg, 16), l = lw;
        if (nc > 1) {
            int t = 0;
            for (int j = 0; j < nc; j++) t += larg[j] + (j < nc - 1 ? 2 : 0);
            return t + 2 > l ? t + 2 : l;
        }
        for (int i = 0; i < n->state.list.item_count; i++) {
            const char *it = n->state.list.items ? n->state.list.items[i] : NULL;
            int e = it ? (int)g_utf8_strlen(it, -1) + 2 : 0;
            if (e > l) l = e;
        }
        return l;
    }
    default: {
        char line[512];
        ligne_feuille(n, 30, 0, line, sizeof line);
        return (int)g_utf8_strlen(line, -1);
    }
    }
}
static int draw_node(WidgetNode *n, int y, int x, int w);

/* Colonne (<vbox>, et l'intérieur d'un <frame>, qui chez gtk3 est une boîte
 * verticale) : hauteurs naturelles d'abord. Si la colonne a reçu plus de
 * lignes (cible), le surplus va aux enfants qui s'étirent (pack_expand, règle
 * de l'étalon, voir widget_vbox.c / widget_frame.c) et la colonne est
 * redessinée : un cadre, une colonne ou une rangée qui remplit sa place la
 * prend toute ; une feuille (bouton, champ, barre) garde sa ligne, centrée
 * dans sa place (une liste qui la remplit, elle, reste en haut). */
static int colonne(WidgetNode *n, int y, int x, int w, int cible)
{
    int nc = n->child_count, used = 0, f0 = g_nfocus;
    int *h = nc > 0 ? g_new0(int, nc) : NULL;
    for (int i = 0; i < nc; i++) {
        h[i] = draw_node(n->children[i], y + used, x, w);
        used += h[i];
    }
    int extra = cible - used, nexp = 0;
    for (int i = 0; i < nc; i++)
        if (h[i] > 0 && n->children[i]->pack_expand) nexp++;
    if (extra > 0 && nexp > 0) {
        effacer_zone(y, x, used, w);
        g_nfocus = f0;
        int oy = y, j = 0;
        for (int i = 0; i < nc; i++) {
            WidgetNode *c = n->children[i];
            int place = h[i];
            if (h[i] > 0 && c->pack_expand)
                place += extra / nexp + (j++ >= nexp - extra % nexp ? 1 : 0);
            int remplit = c->pack_fill && (c->type == WT_VBOX || c->type == WT_EVENTBOX ||
                          c->type == WT_HBOX ||
                          c->type == WT_FRAME || c->type == WT_ASPECTFRAME);
            /* zone de contenu (liste, arbre, texte édité) qui remplit sa
             * place : son contenu commence en HAUT, comme chez gtk3 */
            int zone = c->pack_fill && (c->type == WT_LIST || c->type == WT_TREE ||
                       c->type == WT_TABLE || c->type == WT_EDIT ||
                       c->type == WT_TERMINAL_W || c->type == WT_NOTEBOOK);
            if (place > h[i] && (remplit || (c->pack_fill && c->type == WT_EDIT))) {
                g_haut = place;   /* la zone de texte prend toute sa place */
                draw_node(c, oy, x, w);
            } else if (zone) {
                draw_node(c, oy, x, w);
            } else {
                draw_node(c, oy + (place - h[i]) / 2, x, w);
            }
            oy += place;
        }
        used = cible;
    }
    g_free(h);
    return used;
}

/* Dessine un nœud à (y,x) dans une largeur w ; renvoie le nombre de lignes
 * consommées. Récursif : gère conteneurs et feuilles. */
/* Grisé (sensitive="false", <sensitive>false</sensitive>, disable:) : comme
 * GTK, le widget — ou tout ce que contient un conteneur grisé — ne prend ni le
 * focus ni les touches, et se dessine atténué. Jusqu'ici ce port n'en tenait
 * aucun compte : un bouton grisé s'activait. */
static int insensible(const WidgetNode *n)
{
    for (; n; n = n->parent)
        if (!n->sensitive) return 1;
    return 0;
}

/* Prochain widget qui accepte le focus, dans le sens pas (+1 ou -1) ; depart
 * s'il n'y en a aucun. */
static int focus_suivant(int depart, int pas)
{
    for (int k = 1; k <= g_nfocus; k++) {
        int i = ((depart + pas * k) % g_nfocus + g_nfocus) % g_nfocus;
        if (!insensible(g_focus[i].n)) return i;
    }
    return depart;
}

static int draw_node_corps(WidgetNode *n, int y, int x, int w);

static int draw_node(WidgetNode *n, int y, int x, int w)
{
    int grise = n && !n->sensitive, used;
    if (grise) attron(A_DIM);
    used = draw_node_corps(n, y, x, w);
    if (grise) attroff(A_DIM);
    return used;
}

static int draw_node_corps(WidgetNode *n, int y, int x, int w)
{
    int etirer = g_etirer, cible = g_haut;
    g_etirer = 0; g_haut = 0;
    if (!n || (!n->visible && n->type != WT_WINDOW)) return 0;
    const char *lbl = n->label ? n->label : "";

    switch (n->type) {
    case WT_TIMER:
    /* Images et pixmaps/icônes : OCCULTÉS en terminal. Un backend texte ne peut
     * pas dessiner d'image ; auparavant le nœud tombait dans la feuille par
     * défaut et affichait son CHEMIN de fichier (« /usr/share/icons/… ») en
     * clair, ce qui polluait l'affichage (surtout les icônes en tête de ligne).
     * On ne consomme aucune ligne : rien n'est dessiné. */
    case WT_IMAGE_W:
    case WT_PIXMAP:
        return 0;

    case WT_WINDOW: {
        /* default-height / height-request : la fenêtre a une hauteur, son
         * enfant la remplit (comme gtk3) ; sans hauteur, la naturelle. */
        int used = 0;
        int haut = n->height > 0 ? n->height / PX_PAR_LIGNE : 0;
        if (haut > LINES - 3) haut = LINES - 3;   /* LINES-2 : l'invite de saisie */
        for (int i = 0; i < n->child_count; i++) {
            if (n->child_count == 1 && haut > 0) g_haut = haut;
            used += draw_node(n->children[i], y + used, x, w);
        }
        return used;
    }

    case WT_EXPANDER: {
        /* Un titre « [-] Détails » (ouvert) ou « [+] Détails » (fermé), que
         * Entrée/Espace ouvre et ferme ; le contenu sous le titre s'il est
         * ouvert. Jusqu'à la 2.8.0, le titre n'était pas dessiné. */
        mvprintw(y, x, "[%c] %s", n->state.expander.open ? '-' : '+', lbl);
        if (g_nfocus < 256) {
            g_focus[g_nfocus] = (Focusable){ n, y, x, w };
            if (g_nfocus == g_focus_idx) {
                attron(A_REVERSE);
                mvprintw(y, x, "[%c] %s", n->state.expander.open ? '-' : '+', lbl);
                attroff(A_REVERSE);
            }
            g_nfocus++;
        }
        int used = 1;
        if (n->state.expander.open)
            for (int i = 0; i < n->child_count; i++)
                used += draw_node(n->children[i], y + used, x + 2, w > 2 ? w - 2 : w);
        return used;
    }

    case WT_VBOX: case WT_EVENTBOX:
        return colonne(n, y, x, w, cible);

    case WT_WIZARD: {
        /* L'étape courante, puis la rangée de navigation. Les touches
         * gauche/droite changent d'étape quand la rangée a le focus. */
        int cur = n->state.notebook.current_tab;
        if (cur < 0 || cur >= n->child_count) cur = 0;
        int used = 0;
        if (n->child_count > 0) used = draw_node(n->children[cur], y, x, w);
        if (used < 1) used = 1;
        mvprintw(y + used, x, "[ Precedent ]  [ Suivant ]  [ Terminer ]   etape %d/%d",
                 cur + 1, n->child_count);
        if (g_nfocus < 256) {
            g_focus[g_nfocus] = (Focusable){ n, y + used, x, w };
            if (g_nfocus == g_focus_idx) {
                attron(A_REVERSE);
                mvprintw(y + used, x, "[ Precedent ]  [ Suivant ]  [ Terminer ]");
                attroff(A_REVERSE);
            }
            g_nfocus++;
        }
        return used + 1;
    }

    case WT_STACK: {
        /* Une seule page dessinée. Avec switcher, une rangée « [1] [2] » au
         * -dessus, la page courante en surbrillance. */
        int cur = n->state.notebook.current_tab;
        if (cur < 0 || cur >= n->child_count) cur = 0;
        int used = 0;
        if (n->state.notebook.side) {
            int cx = x;
            for (int i = 0; i < n->child_count; i++) {
                char t[8]; snprintf(t, sizeof t, "[%d]", i + 1);
                if (i == cur) attron(A_REVERSE);
                mvprintw(y, cx, "%s", t);
                if (i == cur) attroff(A_REVERSE);
                cx += (int) strlen(t) + 1;
            }
            used = 1;
        }
        if (n->child_count > 0)
            used += draw_node(n->children[cur], y + used, x, w);
        return used;
    }

    case WT_FLOWBOX: {
        /* Rangement en lignes de N colonnes — même moteur que <grid>, le
         * nombre de colonnes venant de max-children-per-line. */
        int nc = n->state.grid.columns > 0 ? n->state.grid.columns : 1;
        int cw = w / nc; if (cw < 1) cw = 1;
        int used = 0, hmax = 0, col = 0;
        for (int i = 0; i < n->child_count; i++) {
            int h = draw_node(n->children[i], y + used, x + col * cw, cw - 1);
            if (h > hmax) hmax = h;
            if (++col == nc) { used += hmax ? hmax : 1; hmax = 0; col = 0; }
        }
        if (col) used += hmax ? hmax : 1;
        return used;
    }

    case WT_OVERLAY: {
        /* ⚠️ Un terminal ne superpose pas. On dessine le fond, puis les
         * couches l'une sous l'autre, précédées d'un repère : rien n'est
         * perdu, et la dégradation se VOIT. */
        int used = 0;
        for (int i = 0; i < n->child_count; i++) {
            if (i > 0) { mvprintw(y + used, x, "· au-dessus :"); used += 1; }
            used += draw_node(n->children[i], y + used, x, w);
        }
        return used;
    }

    case WT_REVEALER:
        /* Caché = rien du tout, pas même une ligne vide. */
        if (!n->state.toggle.active || n->child_count == 0) return 0;
        return draw_node(n->children[0], y, x, w);

    case WT_TOOLBAR: {
        /* Une rangée soulignée : en terminal, c'est ce qui distingue une barre
         * d'actions d'une simple hbox. Vertical (columns=1) : on empile. */
        if (n->state.grid.columns == 1) {
            int used = 0;
            for (int i = 0; i < n->child_count; i++)
                used += draw_node(n->children[i], y + used, x, w);
            return used;
        }
        int nc = n->child_count; if (nc <= 0) return 0;
        int cw = w / nc, maxh = 1, cx = x;
        for (int i = 0; i < nc; i++) {
            int h = draw_node(n->children[i], y, cx, cw - 1);
            if (h > maxh) maxh = h;
            cx += cw;
        }
        mvhline(y + maxh, x, ACS_HLINE, w);   /* le trait sous la barre */
        return maxh + 1;
    }

    case WT_HBOX: {
        /* Rangée, règle de l'étalon gtk3 (gtk_box_pack_end) transposée en
         * colonnes : chaque enfant a sa largeur naturelle ; le reste va à ceux
         * qui s'étirent (pack_expand, voir widget_hbox.c) ; si personne ne
         * s'étire, la rangée se cale à DROITE, côte à côte. Un enfant étiré
         * sans space-fill est centré dans sa place. homogeneous="true" : des
         * cases ÉGALES sur toute la largeur, chaque enfant centré dans la
         * sienne s'il ne la remplit pas. */
        int nc = n->child_count; if (nc <= 0) return 0;
        int *nat = g_new0(int, nc), *place = g_new0(int, nc), *haut = g_new0(int, nc);
        int f0 = g_nfocus;
        int k = 0, somme = 0, nexp = 0;
        for (int i = 0; i < nc; i++) {
            nat[i] = largeur(n->children[i]);
            if (nat[i] <= 0) continue;
            k++; somme += nat[i];
            if (n->children[i]->pack_expand) nexp++;
        }
        int maxh = 1;
        if (k == 0) { g_free(nat); g_free(place); g_free(haut); return 1; }
        int libre = w - (k - 1) * ESPACE_RANGEE;
        int cx = x, deborde = 0;
        if (n->homogeneous) {
            int cell = libre / k, reste = libre - cell * k, j = 0;
            for (int i = 0; i < nc; i++)
                if (nat[i] > 0) place[i] = cell + (j++ >= k - reste ? 1 : 0);
        } else if (somme <= libre) {
            int extra = libre - somme, j = 0;
            for (int i = 0; i < nc; i++) {
                if (nat[i] <= 0) continue;
                place[i] = nat[i];
                if (nexp && n->children[i]->pack_expand)
                    place[i] += extra / nexp + (j++ >= nexp - extra % nexp ? 1 : 0);
            }
            if (!nexp) cx += extra;                 /* calé à droite */
        } else {
            /* Trop large pour la ligne : cases égales, comme avant. */
            deborde = 1;
            for (int i = 0; i < nc; i++) if (nat[i] > 0) place[i] = libre / k;
        }
        int cx0 = cx;
        for (int i = 0; i < nc; i++) {
            if (nat[i] <= 0) continue;
            WidgetNode *c = n->children[i];
            int h;
            if (deborde) {
                h = draw_node(c, y, cx, place[i]);
            } else if (c->pack_fill) {
                g_etirer = 1;
                h = draw_node(c, y, cx, place[i]);
            } else {
                int cw = nat[i] < place[i] ? nat[i] : place[i];
                h = draw_node(c, y, cx + (place[i] - cw) / 2, cw);
            }
            if (h > maxh) maxh = h;
            haut[i] = h;
            cx += place[i] + ESPACE_RANGEE;
        }
        /* Hauteur de la rangée : celle de son plus haut enfant, ou celle que la
         * colonne lui donne (cible). Comme chez gtk3, chaque enfant occupe
         * toute cette hauteur : un cadre ou une colonne s'y étire, une feuille
         * garde sa ligne, centrée. Redessin si un enfant n'y est pas. */
        int H = cible > maxh ? cible : maxh, refaire = 0;
        for (int i = 0; i < nc; i++)
            if (nat[i] > 0 && haut[i] < H) refaire = 1;
        if (refaire) {
            effacer_zone(y, x, maxh, w);
            g_nfocus = f0;
            cx = cx0;
            for (int i = 0; i < nc; i++) {
                if (nat[i] <= 0) continue;
                WidgetNode *c = n->children[i];
                int conteneur = (c->type == WT_VBOX || c->type == WT_EVENTBOX ||
                                 c->type == WT_FRAME || c->type == WT_ASPECTFRAME);
                int cy = conteneur ? y : y + (H - haut[i]) / 2;
                if (conteneur) g_haut = H;
                if (deborde) {
                    draw_node(c, cy, cx, place[i]);
                } else if (c->pack_fill) {
                    g_etirer = 1;
                    if (conteneur) g_haut = H;
                    draw_node(c, cy, cx, place[i]);
                } else {
                    int cw = nat[i] < place[i] ? nat[i] : place[i];
                    draw_node(c, cy, cx + (place[i] - cw) / 2, cw);
                }
                cx += place[i] + ESPACE_RANGEE;
            }
            maxh = H;
        }
        g_free(nat); g_free(place); g_free(haut);
        return maxh;
    }

    case WT_PANED: {
        /* Deux zones et une frontière dessinée. La poignée se déplace au
         * clavier quand le nœud a le focus : flèches gauche/droite (ou
         * haut/bas en vertical). Un terminal n'a pas de souris à traîner. */
        if (n->child_count < 1) return 0;
        int part = n->state.paned.pixels > 0
                 ? n->state.paned.pixels
                 : (int) ((n->state.paned.vertical ? 10 : w) * n->state.paned.fraction);

        if (n->state.paned.vertical) {
            if (part < 1) part = 1;
            int h1 = draw_node(n->children[0], y, x, w);
            if (h1 < part) h1 = part;
            mvhline(y + h1, x, ACS_HLINE, w);
            int h2 = (n->child_count > 1) ? draw_node(n->children[1], y + h1 + 1, x, w) : 0;
            return h1 + 1 + h2;
        }
        if (part < 4) part = 4;
        if (part > w - 4) part = w - 4;
        int hg = draw_node(n->children[0], y, x, part - 1);
        int hd = (n->child_count > 1) ? draw_node(n->children[1], y, x + part + 1, w - part - 1) : 0;
        int haut = hg > hd ? hg : hd;
        if (haut < 1) haut = 1;
        mvvline(y, x + part, ACS_VLINE, haut);          /* la frontière */
        if (g_nfocus < 256) {
            g_focus[g_nfocus] = (Focusable){ n, y, x + part, 1 };
            if (g_nfocus == g_focus_idx) {
                attron(A_REVERSE);
                mvaddch(y, x + part, ACS_VLINE);        /* poignée saisie */
                attroff(A_REVERSE);
            }
            g_nfocus++;
        }
        return haut;
    }

    case WT_GRID: {
        /* Colonnes de largeur égale, rangées empilées. La hauteur d'une
         * rangée est celle de son enfant le plus haut — sinon un enfant
         * multi-lignes (liste) recouvrirait la rangée suivante. */
        int nc = n->state.grid.columns > 0 ? n->state.grid.columns : 1;
        int cw = w / nc; if (cw < 1) cw = 1;
        int used = 0, hmax = 0, col = 0;
        for (int i = 0; i < n->child_count; i++) {
            int h = draw_node(n->children[i], y + used, x + col * cw, cw - 1);
            if (h > hmax) hmax = h;
            if (++col == nc) { used += hmax ? hmax : 1; hmax = 0; col = 0; }
        }
        if (col) used += hmax ? hmax : 1;   /* dernière rangée incomplète */
        return used;
    }

    case WT_FRAME: case WT_ASPECTFRAME: {
        /* Cadre : bordure + label, enfants à l'intérieur. */
        /* l'intérieur est une colonne (règle de widget_frame.c de l'étalon) */
        int iy = y + 1, ix = x + 1, iw = w - 2;
        int inner = colonne(n, iy, ix, iw, cible > 2 ? cible - 2 : 0);
        int h = inner + 2;
        if (h < cible) h = cible;
        /* bordure */
        mvhline(y, x, ACS_HLINE, w); mvhline(y + h - 1, x, ACS_HLINE, w);
        mvvline(y, x, ACS_VLINE, h); mvvline(y, x + w - 1, ACS_VLINE, h);
        mvaddch(y, x, ACS_ULCORNER); mvaddch(y, x + w - 1, ACS_URCORNER);
        mvaddch(y + h - 1, x, ACS_LLCORNER); mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
        if (*lbl) mvprintw(y, x + 2, " %s ", lbl);
        return h;
    }

    case WT_NOTEBOOK: {
        /* Barre d'onglets + contenu de l'onglet courant. */
        int cur = n->state.notebook.current_tab;
        if (cur < 0 || cur >= n->child_count) cur = 0;
        int tx = x;
        for (int i = 0; i < n->child_count; i++) {
            char t[32];
            const char *tl = (n->state.notebook.tab_labels && n->state.notebook.tab_labels[i])
                             ? n->state.notebook.tab_labels[i] : "•";
            snprintf(t, sizeof t, " %s ", tl);
            if (i == cur) attron(A_REVERSE);
            mvprintw(y, tx, "%s", t);
            if (i == cur) attroff(A_REVERSE);
            tx += (int)strlen(t) + 1;
        }
        int used = draw_node(n->children[cur], y + 2, x, w);
        return used + 2;
    }

    /* ─── Feuilles ─────────────────────────────────────────────────────── */
    default: {
        char line[512];
        int rows = 1;
        if (n->type == WT_SEPARATOR) {
            /* dans une rangée, il reçoit sa largeur naturelle (1) : un trait
             * vertical entre ses voisins */
            if (w <= 1) mvaddch(y, x, ACS_VLINE);
            else mvhline(y, x, ACS_HLINE, w);
            return 1;
        }
        if (n->type == WT_LIST) {
            /* liste multi-lignes ; un <tree> à colonnes : en-têtes, puis
             * lignes alignées */
            int larg[16] = {0};
            int nc = colonnes_mesurer(n, larg, 16);
            char cell[512];
            if (nc > 1 && strchr(lbl, '|')) {
                colonnes_ecrire(lbl, larg, nc, cell, sizeof cell);
                attron(A_BOLD);
                mvprintw(y, x + 2, "%s", cell);
                attroff(A_BOLD);
            } else {
                mvprintw(y, x, "%s", lbl[0] ? lbl : "");
            }
            int base = *lbl ? 1 : 0;
            int cnt = n->state.list.item_count, shown = cnt < 6 ? cnt : 6;
            for (int i = 0; i < shown; i++) {
                int sel = (i == n->state.list.selected_index);
                const char *it = n->state.list.items && n->state.list.items[i] ? n->state.list.items[i] : "";
                if (nc > 1) { colonnes_ecrire(it, larg, nc, cell, sizeof cell); it = cell; }
                if (sel) attron(A_REVERSE);
                mvprintw(y + base + i, x + 2, "%-*s", w > 4 ? w - 4 : 1, it);
                if (sel) attroff(A_REVERSE);
            }
            rows = base + (shown ? shown : 1);
            if (node_focusable(n->type) && g_nfocus < 256) {
                g_focus[g_nfocus++] = (Focusable){ n, y + base, x + 2, w - 4 };
            }
            return rows;
        }
        if (n->type == WT_EDIT) {
            /* Zone de texte : un cadre de plusieurs lignes, comme la zone
             * blanche de l'étalon — au moins 3 lignes de contenu, 10 au plus,
             * toute la hauteur reçue quand la colonne l'étire. Jusqu'ici elle
             * tenait sur une ligne « [texte] » et un texte de plusieurs lignes
             * n'en montrait que le début. Le contenu vit dans
             * state.text.content (voir ligne_feuille). */
            const char *contenu = n->state.text.content ? n->state.text.content : "";
            int nl = 1;
            for (const char *p = contenu; *p; p++)
                if (*p == '\n' && p[1]) nl++;
            int dedans = nl < 3 ? 3 : (nl > 10 ? 10 : nl);
            int h = dedans + 2;
            if (cible > h) h = cible;
            int focused = 0;
            if (g_nfocus < 256) {
                g_focus[g_nfocus] = (Focusable){ n, y + 1, x + 1, w - 2 };
                if (g_nfocus == g_focus_idx) focused = 1;
                g_nfocus++;
            }
            if (focused) attron(A_BOLD);
            mvhline(y, x, ACS_HLINE, w); mvhline(y + h - 1, x, ACS_HLINE, w);
            mvvline(y, x, ACS_VLINE, h); mvvline(y, x + w - 1, ACS_VLINE, h);
            mvaddch(y, x, ACS_ULCORNER); mvaddch(y, x + w - 1, ACS_URCORNER);
            mvaddch(y + h - 1, x, ACS_LLCORNER); mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
            if (focused) attroff(A_BOLD);
            const char *p = contenu;
            for (int i = 0; i < h - 2 && *p; i++) {
                const char *fin = strchr(p, '\n');
                int lg = fin ? (int)(fin - p) : (int)strlen(p);
                if (focused && i == 0) attron(A_REVERSE);
                mvprintw(y + 1 + i, x + 1, "%-*.*s", w - 2 > 0 ? w - 2 : 0, lg < w - 2 ? lg : w - 2, p);
                if (focused && i == 0) attroff(A_REVERSE);
                p = fin ? fin + 1 : p + lg;
            }
            if (!*contenu && focused) mvprintw(y + 1, x + 1, "%*s", w - 2 > 0 ? w - 2 : 0, "");
            return h;
        }
        ligne_feuille(n, w, etirer, line, sizeof line);

        int focused = 0;
        if (node_focusable(n->type) && g_nfocus < 256) {
            g_focus[g_nfocus] = (Focusable){ n, y, x, w };
            if (g_nfocus == g_focus_idx) focused = 1;
            g_nfocus++;
        }
        if (focused) attron(A_REVERSE);
        mvprintw(y, x, "%.*s", w > 0 ? w : (int)strlen(line), line);
        if (focused) attroff(A_REVERSE);
        return rows;
    }
    }
}

/* Édition inline d'un champ texte. */
/* Saisie d'une ligne au bas de l'écran, rangée dans le BON membre de l'union :
 * ⛔ écrire dans entry.buf pour un <edit> écrasait le pointeur state.text.content
 * (fuite, puis free() sur un pointeur fabriqué). */
/* Saisie d'un <password> : une étoile par caractère, jamais le texte.
 * ⛔ echo() + getnstr() recopiait le mot de passe EN CLAIR à l'écran — lisible
 * par-dessus l'épaule et dans tout enregistrement du terminal ; les six autres
 * backends le masquaient. Entrée valide, Échap annule (le champ garde sa
 * valeur), Retour arrière efface un caractère entier, UTF-8 compris.
 * Renvoie 1 si la saisie est validée, 0 si elle est annulée. */
static int saisie_masquee(char *buf, size_t cap)
{
    size_t len = 0;
    int c, valide = 0;

    noecho(); curs_set(1);
    timeout(-1);                          /* attente bloquante pendant la saisie */
    for (;;) {
        c = getch();
        if (c == '\n' || c == '\r' || c == KEY_ENTER) { valide = 1; break; }
        if (c == 27) break;               /* Échap : on abandonne la saisie */
        if (c == KEY_BACKSPACE || c == 127 || c == 8) {
            if (len > 0) {
                do { len--; } while (len > 0 && ((unsigned char) buf[len] & 0xC0) == 0x80);
                int y, x;
                getyx(stdscr, y, x);
                if (x > 0) { mvaddch(y, x - 1, ' '); move(y, x - 1); }
            }
            continue;
        }
        if (c < 32 || c > 255) continue;  /* touches de fonction, flèches : ignorées */
        if (len + 1 >= cap) continue;     /* tampon plein : rien au-delà */
        buf[len++] = (char) c;
        if (((unsigned char) c & 0xC0) != 0x80)
            addch('*');                   /* une étoile par caractère, pas par octet */
    }
    buf[len] = '\0';
    timeout(100);
    curs_set(0);
    return valide;
}

static void edit_field(WidgetNode *n)
{
    char buf[1024] = {0};
    /* invite en bas d'écran */
    move(LINES - 2, 2); clrtoeol();
    mvprintw(LINES - 2, 2, "%s = ", n->label && *n->label ? n->label : "valeur");
    if (n->type == WT_PASSWORD) {
        /* memset appelé par un pointeur volatile : le compilateur ne peut pas
         * retirer l'effacement du secret resté sur la pile. */
        static void *(*const volatile effacer)(void *, int, size_t) = memset;
        if (saisie_masquee(buf, sizeof(buf)))
            snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", buf);
        effacer(buf, 0, sizeof(buf));
        return;
    }
    /* ⛔ La boucle a posé timeout(100) : sans attente bloquante, getnstr rendait
     * ERR au bout de 100 ms, AVANT que quiconque ait tapé — la saisie n'aboutissait
     * jamais, et les lettres tapées ensuite repartaient dans la boucle (« q » quittait). */
    echo(); curs_set(1); timeout(-1);
    if (getnstr(buf, sizeof(buf) - 1) == OK) {
        if (n->type == WT_EDIT) {
            free(n->state.text.content);
            n->state.text.content = strdup(buf);
            n->state.text.len = (int) strlen(buf);
            n->state.text.cap = n->state.text.len + 1;
        } else {
            snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", buf);
        }
    }
    timeout(100);
    noecho(); curs_set(0);
}

/* ─── Thème de fond du terminal ────────────────────────────────────────────
 * Réglable par SERMO_NCURSES_THEME = clair | sombre | bleu | gris. À défaut, on
 * suit SERMO_DARK (1 = sombre, 0 = clair) ; sinon on laisse le défaut du
 * terminal. Sans support des couleurs : no-op. À appeler après chaque
 * initscr() (le fond n'existe que dans une session ncurses ouverte). */
static void st_apply_theme(void)
{
    if (!has_colors()) return;
    const char *t = getenv("SERMO_NCURSES_THEME");
    if (!t || !*t) {
        const char *d = getenv("SERMO_DARK");
        if (!d || !*d) return;                 /* aucune préférence → défaut terminal */
        t = (d[0] != '0') ? "sombre" : "clair";
    }
    start_color();
    short fg = COLOR_WHITE, bg = COLOR_BLACK;   /* défaut = sombre */
    if      (!strcasecmp(t, "clair"))  { fg = COLOR_BLACK; bg = COLOR_WHITE; }
    else if (!strcasecmp(t, "sombre")) { fg = COLOR_WHITE; bg = COLOR_BLACK; }
    else if (!strcasecmp(t, "bleu"))   { fg = COLOR_WHITE; bg = COLOR_BLUE;  }
    else if (!strcasecmp(t, "gris")) {
        /* Gris : redéfinir une couleur libre si le terminal le permet, sinon
         * repli lisible (noir sur blanc). */
        if (can_change_color() && COLORS > 8) { init_color(8, 350, 350, 350); fg = COLOR_WHITE; bg = 8; }
        else { fg = COLOR_BLACK; bg = COLOR_WHITE; }
    }
    /* valeur inconnue → sombre (fg/bg par défaut) */
    init_pair(1, fg, bg);
    bkgd(COLOR_PAIR(1));
    bkgdset(COLOR_PAIR(1));
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Navigateur de fichiers (filechooser)
 *
 *  Un terminal n'a pas de dialogue natif ; il a un répertoire et un clavier.
 *  Flèches pour se déplacer, Entrée pour descendre dans un dossier ou retenir
 *  un fichier, « .. » pour remonter, « h » pour montrer/cacher les fichiers
 *  cachés, Échap pour renoncer sans rien changer.
 * ═══════════════════════════════════════════════════════════════════════════ */
static int fb_is_dir(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static void fb_join(char *out, size_t n, const char *dir, const char *name)
{
    if (dir[0] == '/' && dir[1] == '\0') snprintf(out, n, "/%s", name);
    else                                 snprintf(out, n, "%s/%s", dir, name);
}

/* Remonte d'un cran dans `dir` (sur place). */
static void fb_parent(char *dir)
{
    char *slash = strrchr(dir, '/');
    if (!slash) return;
    if (slash == dir) dir[1] = '\0';      /* « /x » → « / » */
    else              *slash = '\0';
}

static void browse_file(WidgetNode *n)
{
    char dir[PATH_MAX];
    int  folders_only = (n->tooltip && strcasecmp(n->tooltip, "select-folder") == 0);
    int  show_hidden = 0, sel = 0, top = 0;

    /* Point de départ : le chemin courant (son dossier s'il désigne un
     * fichier), sinon le répertoire de travail. */
    if (n->state.entry.buf[0] == '/') {
        snprintf(dir, sizeof dir, "%s", n->state.entry.buf);
        if (!fb_is_dir(dir)) fb_parent(dir);
    } else if (!getcwd(dir, sizeof dir)) {
        snprintf(dir, sizeof dir, "/");
    }
    if (!fb_is_dir(dir)) snprintf(dir, sizeof dir, "/");

    for (;;) {
        struct dirent **ents = NULL;
        int cnt = scandir(dir, &ents, NULL, alphasort);
        if (cnt < 0) {                    /* illisible : remonter ou renoncer */
            if (dir[1] == '\0') return;
            fb_parent(dir); sel = top = 0; continue;
        }

        /* Table des noms retenus : « .. » toujours en tête. */
        char **noms = calloc((size_t) cnt + 1, sizeof *noms);
        int    nn = 0;
        if (!noms) { for (int i = 0; i < cnt; i++) free(ents[i]); free(ents); return; }
        if (dir[1] != '\0') noms[nn++] = strdup("..");
        for (int i = 0; i < cnt; i++) {
            const char *nom = ents[i]->d_name;
            int garder = strcmp(nom, ".") && strcmp(nom, "..")
                      && (show_hidden || nom[0] != '.');
            if (garder && folders_only) {
                char plein[PATH_MAX];
                fb_join(plein, sizeof plein, dir, nom);
                garder = fb_is_dir(plein);
            }
            if (garder) noms[nn++] = strdup(nom);
            free(ents[i]);
        }
        free(ents);

        if (sel >= nn) sel = nn ? nn - 1 : 0;
        int hauteur = LINES - 6 > 3 ? LINES - 6 : 3;
        if (sel < top) top = sel;
        if (sel >= top + hauteur) top = sel - hauteur + 1;

        erase();
        box(stdscr, 0, 0);
        mvprintw(0, 2, " %s ", folders_only ? "Choisir un dossier" : "Choisir un fichier");
        mvprintw(1, 2, "%.*s", COLS - 4, dir);
        mvhline(2, 1, ACS_HLINE, COLS - 2);
        for (int i = 0; i < hauteur && top + i < nn; i++) {
            char plein[PATH_MAX];
            fb_join(plein, sizeof plein, dir, noms[top + i]);
            int rep = (strcmp(noms[top + i], "..") == 0) || fb_is_dir(plein);
            if (top + i == sel) attron(A_REVERSE);
            mvprintw(3 + i, 2, "%-*.*s", COLS - 4, COLS - 4,
                     rep ? noms[top + i] : noms[top + i]);
            if (rep) mvaddch(3 + i, COLS - 3, '/');
            if (top + i == sel) attroff(A_REVERSE);
        }
        if (nn == 0) mvprintw(3, 2, "(vide)");
        mvprintw(LINES - 2, 2,
                 " Entree: %s  ..: remonter  h: caches (%s)  Echap: annuler ",
                 folders_only ? "ouvrir/retenir le dossier" : "ouvrir/choisir",
                 show_hidden ? "montres" : "caches");
        refresh();

        int c = getch();
        int fini = 0, choisi = 0;
        char retenu[PATH_MAX] = "";

        if (c == 27 || c == 'q') fini = 1;                       /* renoncer */
        else if (c == KEY_DOWN || c == '\t') { if (nn) sel = (sel + 1) % nn; }
        else if (c == KEY_UP)   { if (nn) sel = (sel + nn - 1) % nn; }
        else if (c == KEY_NPAGE) { sel += hauteur; if (sel >= nn) sel = nn ? nn - 1 : 0; }
        else if (c == KEY_PPAGE) { sel -= hauteur; if (sel < 0) sel = 0; }
        else if (c == 'h') { show_hidden = !show_hidden; sel = top = 0; }
        else if ((c == '\n' || c == KEY_ENTER) && nn) {
            char plein[PATH_MAX];
            if (strcmp(noms[sel], "..") == 0) {
                fb_parent(dir); sel = top = 0;
            } else {
                fb_join(plein, sizeof plein, dir, noms[sel]);
                if (fb_is_dir(plein) && !folders_only) {
                    snprintf(dir, sizeof dir, "%s", plein); sel = top = 0;
                } else {
                    snprintf(retenu, sizeof retenu, "%s", plein);
                    choisi = 1; fini = 1;
                }
            }
        }

        for (int i = 0; i < nn; i++) free(noms[i]);
        free(noms);

        if (fini) {
            if (choisi)
                snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", retenu);
            return;
        }
    }
}

/* Avance la phase de chaque <pulse> de l'arbre (une frame = un pas). */
static void pulse_avance(WidgetNode *n)
{
    if (!n) return;
    if (n->type == WT_PULSE)
        n->state.spinner.angle += (float) (n->state.spinner.speed ? n->state.spinner.speed : 1.0);
    for (int i = 0; i < n->child_count; i++) pulse_avance(n->children[i]);
}

/* ─── Ouverture et fermeture de l'écran ────────────────────────────────────
 * L'écran se dessine sur la sortie standard quand c'est un terminal. Sinon
 * (sortie capturée par $(…), envoyée dans un fichier ou un tube) il se dessine
 * sur le terminal de contrôle : la capture ne reçoit que les variables.
 * newterm() et non initscr() : initscr() écrit toujours sur la sortie standard,
 * et arrête le programme s'il ne peut pas ouvrir le terminal. */
static FILE *g_tty;          /* /dev/tty, quand la sortie standard est prise */
static int   g_ecran;        /* 1 dès que ncurses tient un terminal */

static void ecran_modes(void)
{
    cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    st_apply_theme();
    timeout(100);
}

/* Rend le terminal à la sortie du programme. Le cœur sort par exit() dès qu'une
 * action « exit:… » est jouée (minuterie, case à cocher, menu…) : sans cela le
 * terminal restait sans écho ni mode ligne après le dialogue. */
static void ecran_fermer(void)
{
    if (g_ecran && !isendwin()) endwin();
}

/* Crochet du contrat : le cœur l'appelle juste avant d'écrire les variables et
 * EXIT=… (action « exit:… », puis exit()). L'écran est rendu d'abord : quand la
 * sortie standard est le terminal, les variables s'écrivent alors sur l'écran
 * normal et restent visibles, au lieu de partir avec l'écran alterné. */
void sermo_be_before_export(void)
{
    ecran_fermer();
    fflush(g_tty ? g_tty : stdout);
}

/* Rend 1 si l'écran est ouvert, 0 s'il n'y a pas de terminal où dessiner. */
static int ecran_ouvrir(void)
{
    FILE *sortie = stdout;
    if (!isatty(STDOUT_FILENO)) {
#ifdef _WIN32
        /* Windows : le terminal de contrôle s'appelle CONOUT$. */
        g_tty = fopen("CONOUT$", "w");
#else
        g_tty = fopen("/dev/tty", "w");
#endif
        if (!g_tty) return 0;
        sortie = g_tty;
    }
    if (!newterm(NULL, sortie, stdin)) {
        if (g_tty) { fclose(g_tty); g_tty = NULL; }
        return 0;
    }
    g_ecran = 1;
    atexit(ecran_fermer);
    ecran_modes();
    return 1;
}

static int run_interactive(WidgetNode *root, const char *title)
{
    g_focus_idx = 0;

    for (;;) {
        erase();
        box(stdscr, 0, 0);
        /* Un terminal ne peut pas porter l'icône d'application : la marque
         * « HD » est écrite dans le filet du haut, à la place du logo. */
        mvprintw(0, 2, " HD ");
        mvprintw(0, 6, "%s ", (title && *title) ? title : "sermo");
        g_nfocus = 0;
        draw_node(root, 1, 2, COLS - 4);
        if (g_focus_idx >= g_nfocus) g_focus_idx = g_nfocus ? g_nfocus - 1 : 0;
        if (g_nfocus && insensible(g_focus[g_focus_idx].n)) {
            int i = focus_suivant(g_focus_idx, 1);
            if (i != g_focus_idx) { g_focus_idx = i; continue; }   /* redessiner là */
        }
        mvprintw(LINES - 1, 2, " Tab/fleches: naviguer  Entree/Espace: activer  q: quitter ");
        refresh();

        fire_timers(root);     /* un exit:… sort ici via le cœur */
        servir_barres(root);   /* les barres suivent leur commande */
        pulse_avance(root);    /* barres indéterminées : une frame de plus */

        int c = getch();
        if (c == ERR) continue;
        if (c == 'q' || c == 27) { endwin(); return export_abort(); }

        if (c == '\t' || c == KEY_DOWN)      g_focus_idx = g_nfocus ? focus_suivant(g_focus_idx, 1) : 0;
        else if (c == KEY_UP || c == KEY_BTAB) g_focus_idx = g_nfocus ? focus_suivant(g_focus_idx, -1) : 0;
        else if (g_nfocus && insensible(g_focus[g_focus_idx].n))
            continue;   /* tout est grisé : aucune touche n'agit */
        else if ((c == '\n' || c == ' ' || c == KEY_ENTER) && g_nfocus > 0) {
            WidgetNode *n = g_focus[g_focus_idx].n;
            switch (n->type) {
            case WT_BUTTON:
                if (n->action && *n->action) {
                    endwin();
                    declencher(n, "clicked", NULL);   /* exit:… peut sortir ici */
                    ecran_modes();   /* le prochain refresh() reprend l'écran */
                } else if (!(n->var_name && strcmp(n->var_name, "__fontbutton__") == 0)) {
                    /* <fontbutton> est construit comme un bouton marqué __fontbutton__
                     * (widget_fontbutton.c) : il ne doit pas fermer le dialogue. */
                    /* gtkdialog : un bouton nu (<button ok>, <button cancel>…) ferme le
                     * dialogue, EXIT = valeur de famille sinon libellé — comme sdl3.
                     * Ici rien ne se passait : le bouton OK d'un formulaire ne sortait pas. */
                    endwin();
                    action_exitprogram((GtkWidget *)n,
                        (char *)(n->tooltip ? n->tooltip
                                 : (n->label && n->label[0] ? n->label : "OK")));
                }
                break;
            case WT_EXPANDER:
                n->state.expander.open = !n->state.expander.open;
                if (n->action && *n->action) declencher(n, "activate", NULL);
                break;
            case WT_CHECKBOX: case WT_TOGGLEBUTTON: case WT_RADIOBUTTON: case WT_SWITCH:
                basculer(n);   /* exit:… peut sortir ici */
                break;
            case WT_ENTRY: case WT_EDIT: case WT_PASSWORD:
                edit_field(n); break;
            case WT_MENUBUTTON: {
                /* Entrée/Espace fait défiler les choix : un terminal n'a pas
                 * de menu flottant, mais il a un cycle. */
                int i, cur = -1;
                for (i = 0; i < n->child_count; i++) {
                    const char *l = n->children[i]->label;
                    if (l && !strcmp(l, n->state.entry.buf)) { cur = i; break; }
                }
                if (n->child_count > 0) {
                    WidgetNode *choisi = n->children[(cur + 1) % n->child_count];
                    snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s",
                             choisi->label ? choisi->label : "");
                    if (choisi->action && *choisi->action)
                        declencher(choisi, "activate", NULL);
                }
                break;
            }
            case WT_LINKBUTTON:
                sermo_open_uri(n->state.entry.buf);
                if (n->action && *n->action) declencher(n, NULL, NULL);
                break;
            case WT_FILECHOOSER:
                browse_file(n);
                if (n->action && *n->action) declencher(n, NULL, NULL);
                break;
            case WT_SEARCHENTRY: {
                char buf[256] = {0};
                move(LINES - 2, 2); clrtoeol(); mvprintw(LINES - 2, 2, "rechercher: ");
                echo(); curs_set(1); timeout(-1);        /* même piège que edit_field */
                getnstr(buf, sizeof(buf) - 1);
                timeout(100); noecho(); curs_set(0);
                snprintf(n->state.list.filter, sizeof(n->state.list.filter), "%s", buf);
                if (n->action && *n->action) declencher(n, NULL, NULL);
                break;
            }
            case WT_LIST: case WT_COMBOBOX:
                if (n->state.list.item_count > 0) {
                    int avant = n->state.list.selected_index;
                    n->state.list.selected_index =
                        (n->state.list.selected_index + 1) % n->state.list.item_count;
                    if (n->state.list.selected_index != avant)
                        liste_selection_changee(n);   /* WT_LIST seulement */
                }
                break;
            case WT_SPINBUTTON:
                n->state.spin.value += 1;
                if (n->state.spin.max > n->state.spin.min && n->state.spin.value > n->state.spin.max)
                    n->state.spin.value = n->state.spin.max;
                break;
            case WT_HSCALE: case WT_VSCALE: {
                double step = n->state.scale.step ? n->state.scale.step : 1;
                n->state.scale.value += step;
                if (n->state.scale.max > n->state.scale.min && n->state.scale.value > n->state.scale.max)
                    n->state.scale.value = n->state.scale.max;
                break;
            }
            default: break;
            }
        } else if (c == KEY_RIGHT && g_nfocus &&
                   g_focus[g_focus_idx].n->type == WT_WIZARD) {
            WidgetNode *n = g_focus[g_focus_idx].n;
            if (n->state.notebook.current_tab < n->child_count - 1)
                n->state.notebook.current_tab++;
        } else if (c == KEY_RIGHT && g_nfocus &&
                   g_focus[g_focus_idx].n->type == WT_PANED) {
            WidgetNode *n = g_focus[g_focus_idx].n;
            if (n->state.paned.resizable) {
                if (n->state.paned.pixels > 0) n->state.paned.pixels += 2;
                else { n->state.paned.fraction += 0.05;
                       if (n->state.paned.fraction > 0.9) n->state.paned.fraction = 0.9; }
            }
        } else if (c == KEY_LEFT) {   /* décrémenter échelles/spin */
            WidgetNode *n = g_nfocus ? g_focus[g_focus_idx].n : NULL;
            if (n && n->type == WT_WIZARD) {
                if (n->state.notebook.current_tab > 0) n->state.notebook.current_tab--;
                continue;
            }
            if (n && n->type == WT_PANED) {
                if (n->state.paned.resizable) {
                    if (n->state.paned.pixels > 2) n->state.paned.pixels -= 2;
                    else if (n->state.paned.pixels == 0) {
                        n->state.paned.fraction -= 0.05;
                        if (n->state.paned.fraction < 0.1) n->state.paned.fraction = 0.1;
                    }
                }
                continue;
            }
            if (n && (n->type == WT_HSCALE || n->type == WT_VSCALE)) {
                double step = n->state.scale.step ? n->state.scale.step : 1;
                n->state.scale.value -= step;
                if (n->state.scale.value < n->state.scale.min) n->state.scale.value = n->state.scale.min;
            } else if (n && n->type == WT_SPINBUTTON) {
                n->state.spin.value -= 1;
                if (n->state.spin.value < n->state.spin.min) n->state.spin.value = n->state.spin.min;
            }
        }
    }
}

/* ─── Point d'entrée ───────────────────────────────────────────────────────── */
int render_loop(WidgetNode *root, const char *title, int win_w, int win_h)
{
    (void)win_w; (void)win_h;
    if (!root) return 0;
    /* Sans écran : sur demande, ou quand personne ne peut taper (entrée
     * standard qui n'est pas un terminal), ou faute de terminal où dessiner.
     * Une sortie standard capturée ne suffit PAS à renoncer à l'écran. */
    if (getenv("SERMO_NCURSES_BATCH") || !isatty(STDIN_FILENO) || !ecran_ouvrir()) {
        listes_chargees(root);   /* un « exit:… » peut sortir ici, via le cœur */
        return run_headless(root);
    }
    listes_chargees(root);       /* l'écran ouvert : une sortie le rend propre */
    return run_interactive(root, title);
}
