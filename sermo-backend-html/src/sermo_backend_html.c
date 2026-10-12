/* sermo_backend_html.c — hook d'init toolkit + ABI neutre du backend HTML statique.
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Le cœur (libsermocore, variante NEUTRE) pilote parse/automaton/variables/
 * actions et assemble l'arbre de widgets via une ABI « neutre » nommée qt6_*
 * (héritée du premier port neutre). Ce fichier la réalise sur le modèle
 * arbre-de-WidgetNode, comme les backends sdl3/ncurses ; gtk_main
 * (= sermo_be_run_loop) exporte D'ABORD l'arbre en HTML statique
 * (render_html_export, garanti même si un <timer> sort ensuite le
 * programme), PUIS rejoue les <timer> une fois — comme les autres
 * backends en mode sans affichage (headless CI) — pour rester comparable
 * au banc de comportement de la famille (tests/comportement/). Un
 * <timer><action>exit:...</action></timer> imprime alors les variables
 * (print_variables, via action_exitprogram du cœur) et termine ; sans
 * timer, repli minimal ci-dessous. La page HTML produite reste, elle,
 * sans JavaScript ni logique serveur : ce rejeu ne touche que le
 * PROCESSUS d'export, jamais la sortie. */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "sermo_backend.h"
#include "dialog_state.h"      /* WidgetNode, widget_node_add_child, WT_* */
#include "render_html.h"       /* render_html_export(), html_mode() */
#include "serve_http.h"        /* html_serve_once() */
#include "variables.h"         /* variables_export_all(), print_variables() */
#include "actions.h"           /* execute_action() */
#include "attributes.h"        /* AttributeSet, ATTR_ACTION */
#include "sermo_progress.h"      /* barres de progression lues au fil de l'eau */
#include "jouer_actions.h"     /* jouer_actions() : actions avec leur fonction */

/* Globales que le cœur déclare extern (visibilité des widgets). */
GList *widget_show_list = NULL;
GList *widget_hide_list = NULL;

/* Racine capturée par widget_window_create (nom hérité du port neutre). */
WidgetNode *sdl3_root_window = NULL;

extern int option_print_ir;   /* gboolean (gint) défini dans le cœur */

void sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)
{
    (void)argc; (void)argv; (void)print_ir;
    /* Rien à ouvrir : l'export HTML n'a pas de session graphique. */
}

/* ─── ABI neutre qt6_* sur l'arbre WidgetNode ──────────────────────────────── */
void sermo_be_widget_show(void *w)
{ WidgetNode *n = (WidgetNode *)w; if (n) n->visible = TRUE; }

void sermo_be_widget_hide(void *w)
{ WidgetNode *n = (WidgetNode *)w; if (n) n->visible = FALSE; }

void sermo_be_widget_set_sensitive(void *w, int sensitive)
{ WidgetNode *n = (WidgetNode *)w; if (n) n->sensitive = (sensitive != 0); }

void sermo_be_widget_redraw(void *w)
{ (void)w; /* export en un seul passage : rien à redessiner */ }

void sermo_be_container_add(void *container, void *child)
{
    WidgetNode *c = (WidgetNode *)container, *ch = (WidgetNode *)child;
    if (c && ch) widget_node_add_child(c, ch);
}

void *sermo_be_container_child0(void *container)
{
    WidgetNode *c = (WidgetNode *)container;
    return (c && c->child_count > 0) ? c->children[0] : NULL;
}

void sermo_be_window_move(void *w, int x, int y)
{ (void)w; (void)x; (void)y; /* sans objet pour une page HTML */ }

void *sermo_be_scroll_new(int w, int h)
{
    WidgetNode *n = widget_node_new(WT_VBOX, NULL, NULL);
    if (n) { n->width = w; n->height = h; }
    return n;
}

/* ─── Cycle de vie « application » ─────────────────────────────────────────── */
void sermo_be_app_init(int *argc, char ***argv)
{ (void)argc; (void)argv; }

/* Joue les actions d'un nœud : toutes, chacune avec sa fonction, quand le
 * créateur a gardé le jeu d'attributs ; sinon l'unique commande du nœud.
 * Même règle que le port ncurses (voir jouer_actions.h). */
static void declencher(WidgetNode *n, const char *signal1, const char *signal2)
{
    if (!n) return;
    if (n->actions_attr)
        jouer_actions((GtkWidget *)n, (AttributeSet *)n->actions_attr, signal1, signal2);
    else if (n->action && *n->action)
        execute_action((GtkWidget *)n, n->action, NULL);
}

/* La sélection posée au chargement d'une <list> est jouée une fois, l'arbre
 * entier construit : les variables que l'action vise existent alors. Seule une
 * vraie <list> garde son jeu d'actions ; un <tree> (WT_LIST aussi) non. */
static void listes_chargees(WidgetNode *n)
{
    if (!n) return;
    if (n->type == WT_LIST && n->actions_attr && n->state.list.selected_index >= 0)
        declencher(n, "selection-changed", "row-selected");
    for (int i = 0; i < n->child_count; i++) listes_chargees(n->children[i]);
}

/* Rejoue les actions des <timer> de l'arbre, une fois. Un « exit:... » sort
 * via action_exitprogram (print_variables + exit), sans repasser par ici. */
static void lister_timers(WidgetNode *n, WidgetNode ***t, int *nb, int *cap)
{
    if (!n) return;
    if (n->type == WT_TIMER && n->sensitive) {
        if (*nb == *cap) { *cap = *cap ? *cap * 2 : 8; *t = realloc(*t, (size_t)*cap * sizeof **t); }
        if (*t) (*t)[(*nb)++] = n;
    }
    for (int i = 0; i < n->child_count; i++) lister_timers(n->children[i], t, nb, cap);
}
static int intervalle_ms(const WidgetNode *n)
{
    int ms = (n->label && *n->label) ? atoi(n->label) : 1000;
    return ms > 0 ? ms : 1000;
}
/* Chacune une fois, de la plus courte à la plus longue (tri stable : deux
 * délais égaux gardent l'ordre du document) — l'ordre où elles partiraient
 * dans une fenêtre. Jusqu'à la 2.8.1 : l'ordre du document, et une minuterie
 * de 8 s fermait le dialogue avant celle d'une seconde. */
static void fire_timers_once(WidgetNode *root)
{
    WidgetNode **t = NULL;
    int nb = 0, cap = 0;
    lister_timers(root, &t, &nb, &cap);
    for (int i = 1; i < nb; i++)
        for (int j = i; j > 0 && intervalle_ms(t[j - 1]) > intervalle_ms(t[j]); j--) {
            WidgetNode *x = t[j]; t[j] = t[j - 1]; t[j - 1] = x;
        }
    for (int i = 0; i < nb; i++) declencher(t[i], "tick", NULL);
    free(t);
}

/* Barres de progression : relever ce que leurs commandes ont écrit. Rend le
 * nombre de lectures encore ouvertes. Sans écran, rien ne presse : on lit
 * chaque commande jusqu'au bout (la première ligne à 100 déclenche ses
 * actions, un « exit:… » sort ici). */
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

int sermo_be_run_loop(void)
{
    WidgetNode *root = sdl3_root_window;
    if (option_print_ir || !root) return 0;   /* headless : le cœur imprime l'IR */

    const char *title = (root->label && *root->label) ? root->label : "htmlsermo";

    /* Mode « serve » : la page n'est pas ecrite sur disque, elle est servie sur
     * la boucle locale et le formulaire renvoie les valeurs. C'est le cycle de
     * vie d'une fenetre de dialogue, transpose au navigateur — donc on ne
     * rejoue PAS les timers et on ne sort pas par le repli EXIT="abort". */
    if (html_mode() == 2 /* MODE_SERVE */) {
        char *buf = NULL; size_t blen = 0;
        FILE *mem = open_memstream(&buf, &blen);
        if (!mem) { perror("htmlsermo: open_memstream"); exit(EXIT_FAILURE); }
        render_html_write(mem, root, title);
        fclose(mem);
        int rc = html_serve_once(buf, (unsigned long)blen);
        free(buf);
        if (rc != 0) { printf("EXIT=\"abort\"\n"); g_setenv("EXIT", "abort", TRUE); fflush(stdout); }
        exit(EXIT_SUCCESS);
    }

    /* 1) L'export HTML d'abord, INCONDITIONNELLEMENT : c'est la promesse du
     * backend, elle ne doit jamais dépendre de ce qu'un <timer> fait ensuite. */
    render_html_export(root, title);

    /* 2) Rejeu des timers, comme les autres backends en mode headless.
     * Si l'un déclenche « exit:... », action_exitprogram imprime les
     * variables et termine ici — on ne revient pas. */
    listes_chargees(root);
    while (servir_barres(root) > 0)
        usleep(20000);
    fire_timers_once(root);

    /* 3) Repli si aucun timer n'a fait sortir le programme : mêmes lignes
     * que les autres backends dans ce cas (variables + EXIT="abort"). */
    print_variables(NULL);
    printf("EXIT=\"abort\"\n");
    g_setenv("EXIT", "abort", TRUE);   /* --do lit EXIT dans l'environnement (manuel §3.2) : il n'y était pas. */
    fflush(stdout);
    exit(EXIT_SUCCESS);
}

void sermo_be_app_quit(void)
{ /* pas de boucle à interrompre */ }
