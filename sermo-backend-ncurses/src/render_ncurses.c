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
 *  - HEADLESS (pas de tty, ou SERMO_NCURSES_BATCH=1) : n'ouvre pas ncurses ;
 *    exécute la logique (timers → actions ; un « exit:… » fait exporter et
 *    sortir le cœur). Sert au scripting ET aux bancs (sans terminal).
 *  - INTERACTIF (tty) : moteur de disposition récursif (vbox empile, hbox aligne,
 *    frame encadre, notebook onglets) + rendu riche des widgets, navigation
 *    Tab/flèches, activation Entrée/Espace.
 */
#include <ncurses.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
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

/* Fourni par le cœur (actions.c), non exporté par ses en-têtes — comme dans sdl3. */
void action_exitprogram(GtkWidget *widget, char *string);

/* ═══════════════════════════════════════════════════════════════════════════
 *  Déclenchement des <timer> + export (partagé headless/interactif)
 * ═══════════════════════════════════════════════════════════════════════════ */
static void fire_timers(WidgetNode *node)
{
    if (!node) return;
    if (node->type == WT_TIMER) {
        if (node->actions_attr) {
            AttributeSet *aa = (AttributeSet *)node->actions_attr;
            GList *el = NULL;
            gchar *fn = attributeset_get_first(&el, aa, ATTR_ACTION);
            while (fn) { if (*fn) execute_action((GtkWidget *)node, fn, NULL);
                        fn = attributeset_get_next(&el, aa, ATTR_ACTION); }
        } else if (node->action && *node->action) {
            execute_action((GtkWidget *)node, node->action, NULL);
        }
    }
    for (int i = 0; i < node->child_count; i++) fire_timers(node->children[i]);
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
    fflush(stdout);
    return 0;
}

static int run_headless(WidgetNode *root)
{
    /* Sans écran, rien ne presse : chaque barre lit sa commande jusqu'au bout
     * (la première ligne à 100 déclenche ses actions, un « exit:… » sort ici),
     * puis les minuteries partent une fois. Une commande sans fin bloque, comme
     * la lecture d'un seul tenant le faisait déjà jusqu'à la 2.6.8. */
    while (servir_barres(root) > 0)
        usleep(20000);
    fire_timers(root);      /* un « exit:… » sort ici via action_exitprogram */
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

/* Dessine un nœud à (y,x) dans une largeur w ; renvoie le nombre de lignes
 * consommées. Récursif : gère conteneurs et feuilles. */
static int draw_node(WidgetNode *n, int y, int x, int w)
{
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
        int used = 0;
        for (int i = 0; i < n->child_count; i++)
            used += draw_node(n->children[i], y + used, x, w);
        return used;
    }

    case WT_VBOX: case WT_EXPANDER: case WT_EVENTBOX: {
        int used = 0;
        for (int i = 0; i < n->child_count; i++)
            used += draw_node(n->children[i], y + used, x, w);
        return used ? used : 0;
    }

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
        int nc = n->child_count; if (nc <= 0) return 0;
        int cw = w / nc, maxh = 1, cx = x;
        for (int i = 0; i < nc; i++) {
            int h = draw_node(n->children[i], y, cx, cw);
            if (h > maxh) maxh = h;
            cx += cw;
        }
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
        int inner = 0, iy = y + 1, ix = x + 1, iw = w - 2;
        for (int i = 0; i < n->child_count; i++)
            inner += draw_node(n->children[i], iy + inner, ix, iw);
        int h = inner + 2;
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
        switch (n->type) {
        case WT_TEXT: case WT_STATUSBAR:
            snprintf(line, sizeof line, "%s", lbl); break;
        case WT_SEPARATOR:
            mvhline(y, x, ACS_HLINE, w); return 1;
        case WT_BUTTON:
            snprintf(line, sizeof line, "[ %s ]", lbl[0] ? lbl : "OK"); break;
        case WT_CHECKBOX: case WT_TOGGLEBUTTON: case WT_SWITCH:
            snprintf(line, sizeof line, "[%c] %s", n->state.toggle.active ? 'x' : ' ', lbl); break;
        case WT_RADIOBUTTON:
            snprintf(line, sizeof line, "(%c) %s", n->state.toggle.active ? '*' : ' ', lbl); break;
        case WT_ENTRY:
            snprintf(line, sizeof line, "%s[%-16s]", lbl[0] ? lbl : "", n->state.entry.buf); break;
        case WT_EDIT: {
            /* ⛔ Le contenu d'un <edit> vit dans state.text.content, un
             * POINTEUR de tas — pas dans state.entry.buf. Les deux occupent la
             * même place dans l'union : lire entry.buf ici revenait à afficher
             * les octets du pointeur, puis à lire au-delà jusqu'au premier
             * zéro (débordement de lecture). Mesuré : « ��jU » à l'écran. */
            const char *contenu = n->state.text.content ? n->state.text.content : "";
            snprintf(line, sizeof line, "%s[%-16s]", lbl[0] ? lbl : "", contenu);
            break;
        }
        case WT_SEARCHENTRY:
            snprintf(line, sizeof line, "%s[%-16s]?", lbl[0] ? lbl : "", n->state.list.filter); break;
        case WT_PASSWORD: {
            int L = (int)strlen(n->state.entry.buf), k = L < 16 ? L : 16;
            char st[24]; memset(st, '*', k); st[k] = '\0';
            snprintf(line, sizeof line, "%s[%-16s]", lbl[0] ? lbl : "", st); break;
        }
        case WT_SPINBUTTON:
            snprintf(line, sizeof line, "%s[%d]", lbl[0] ? lbl : "", (int)n->state.spin.value); break;
        case WT_PROGRESSBAR: case WT_LEVELBAR: {
            char bar[160];
            double mx = (n->state.scale.max > n->state.scale.min) ? n->state.scale.max : 1.0;
            draw_bar(y, x, w > 30 ? 30 : w, n->state.scale.value / (mx ? mx : 1.0), bar, sizeof bar);
            snprintf(line, sizeof line, "%s%s", lbl[0] ? lbl : "", bar); break;
        }
        case WT_HSCALE: case WT_VSCALE: {
            char bar[160];
            double lo = n->state.scale.min, hi = n->state.scale.max, v = n->state.scale.value;
            double frac = (hi > lo) ? (v - lo) / (hi - lo) : 0;
            draw_bar(y, x, w > 24 ? 24 : w, frac, bar, sizeof bar);
            snprintf(line, sizeof line, "%s%s %.6g", lbl[0] ? lbl : "", bar, v); break;
        }
        case WT_LIST: case WT_COMBOBOX: {
            if (n->type == WT_COMBOBOX) {
                int si = n->state.list.selected_index;
                const char *it = (si >= 0 && si < n->state.list.item_count &&
                                  n->state.list.items && n->state.list.items[si])
                                 ? n->state.list.items[si] : "";
                snprintf(line, sizeof line, "%s< %s >", lbl[0] ? lbl : "", it);
            } else {
                /* liste multi-lignes */
                mvprintw(y, x, "%s", lbl[0] ? lbl : "");
                int base = *lbl ? 1 : 0;
                int cnt = n->state.list.item_count, shown = cnt < 6 ? cnt : 6;
                for (int i = 0; i < shown; i++) {
                    int sel = (i == n->state.list.selected_index);
                    if (sel) attron(A_REVERSE);
                    mvprintw(y + base + i, x + 2, "%-*s",
                             w > 4 ? w - 4 : 1,
                             n->state.list.items && n->state.list.items[i] ? n->state.list.items[i] : "");
                    if (sel) attroff(A_REVERSE);
                }
                rows = base + (shown ? shown : 1);
                if (node_focusable(n->type) && g_nfocus < 256) {
                    g_focus[g_nfocus++] = (Focusable){ n, y + base, x + 2, w - 4 };
                }
                return rows;
            }
            break;
        }
        case WT_INFOBAR:
            snprintf(line, sizeof line, "[%s] %s", n->state.infobar.message_type, lbl); break;
        case WT_MENUBUTTON:
            /* Le libellé, puis le choix courant entre chevrons — en terminal
             * un menu déroulant se lit mieux ainsi qu'il ne se déroule. */
            snprintf(line, sizeof line, "[ %s \u25be ] %s", lbl[0] ? lbl : "Menu",
                     n->state.entry.buf);
            break;
        case WT_LINKBUTTON:
            /* Le libellé, puis l'URI entre chevrons — en terminal le lien ne
             * se « survole » pas : l'adresse doit être lisible telle quelle. */
            if (n->state.entry.buf[0] && strcmp(lbl, n->state.entry.buf) != 0)
                snprintf(line, sizeof line, "%s <%s>", lbl, n->state.entry.buf);
            else
                snprintf(line, sizeof line, "<%s>", n->state.entry.buf);
            break;
        case WT_FILECHOOSER:
            snprintf(line, sizeof line, "%s[%-24s]", lbl[0] ? lbl : "",
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
            snprintf(line, sizeof line, "%s[%s]", lbl[0] ? lbl : "", bar);
            break;
        }
        case WT_CALENDAR:
            snprintf(line, sizeof line, "%s<%04d-%02d-%02d>", lbl[0] ? lbl : "",
                     n->state.calendar.year, n->state.calendar.month, n->state.calendar.day); break;
        default:
            snprintf(line, sizeof line, "%s", lbl); break;
        }

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

static int run_interactive(WidgetNode *root, const char *title)
{
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    st_apply_theme();
    timeout(100);
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
        mvprintw(LINES - 1, 2, " Tab/fleches: naviguer  Entree/Espace: activer  q: quitter ");
        refresh();

        fire_timers(root);     /* un exit:… sort ici via le cœur */
        servir_barres(root);   /* les barres suivent leur commande */
        pulse_avance(root);    /* barres indéterminées : une frame de plus */

        int c = getch();
        if (c == ERR) continue;
        if (c == 'q' || c == 27) { endwin(); return export_abort(); }

        if (c == '\t' || c == KEY_DOWN)      g_focus_idx = g_nfocus ? (g_focus_idx + 1) % g_nfocus : 0;
        else if (c == KEY_UP || c == KEY_BTAB) g_focus_idx = g_nfocus ? (g_focus_idx + g_nfocus - 1) % g_nfocus : 0;
        else if ((c == '\n' || c == ' ' || c == KEY_ENTER) && g_nfocus > 0) {
            WidgetNode *n = g_focus[g_focus_idx].n;
            switch (n->type) {
            case WT_BUTTON:
                if (n->action && *n->action) {
                    endwin();
                    execute_action((GtkWidget *)n, n->action, NULL);   /* exit:… peut sortir ici */
                    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0); st_apply_theme(); timeout(100);
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
            case WT_CHECKBOX: case WT_TOGGLEBUTTON: case WT_SWITCH: case WT_RADIOBUTTON:
                n->state.toggle.active = !n->state.toggle.active;
                if (n->action && *n->action) execute_action((GtkWidget *)n, n->action, NULL);
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
                        execute_action((GtkWidget *)choisi, choisi->action, NULL);
                }
                break;
            }
            case WT_LINKBUTTON:
                sermo_open_uri(n->state.entry.buf);
                if (n->action && *n->action) execute_action((GtkWidget *)n, n->action, NULL);
                break;
            case WT_FILECHOOSER:
                browse_file(n);
                if (n->action && *n->action) execute_action((GtkWidget *)n, n->action, NULL);
                break;
            case WT_SEARCHENTRY: {
                char buf[256] = {0};
                move(LINES - 2, 2); clrtoeol(); mvprintw(LINES - 2, 2, "rechercher: ");
                echo(); curs_set(1); timeout(-1);        /* même piège que edit_field */
                getnstr(buf, sizeof(buf) - 1);
                timeout(100); noecho(); curs_set(0);
                snprintf(n->state.list.filter, sizeof(n->state.list.filter), "%s", buf);
                if (n->action && *n->action) execute_action((GtkWidget *)n, n->action, NULL);
                break;
            }
            case WT_LIST: case WT_COMBOBOX:
                if (n->state.list.item_count > 0)
                    n->state.list.selected_index =
                        (n->state.list.selected_index + 1) % n->state.list.item_count;
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
    int headless = getenv("SERMO_NCURSES_BATCH")
                || !isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO);
    return headless ? run_headless(root) : run_interactive(root, title);
}
