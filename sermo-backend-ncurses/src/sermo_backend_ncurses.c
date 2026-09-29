/* sermo_backend_ncurses.c — hook d'init toolkit + ABI neutre du backend ncurses.
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Le cœur (libsermocore, variante NEUTRE) pilote parse/automaton/variables/
 * actions et assemble l'arbre de widgets via une ABI « neutre » nommée qt6_*
 * (héritée du premier port neutre). Ce fichier la réalise sur le modèle
 * arbre-de-WidgetNode, comme le backend sdl3 ; gtk_main (= sermo_be_run_loop) entre
 * dans la boucle ncurses (render_loop). En --print-ir : aucun affichage. */
#include <stdio.h>
#include <stdlib.h>

#include "sermo-contract.h"   /* contrat cœur <-> backend (MIT) */
#include "dialog_state.h"      /* WidgetNode, widget_node_add_child, WT_* */
#include "render_ncurses.h"    /* render_loop() */

/* Globales que le cœur déclare extern (visibilité des widgets). */
GList *widget_show_list = NULL;
GList *widget_hide_list = NULL;

/* Racine capturée par widget_window_create (nom hérité du port neutre). */
WidgetNode *sdl3_root_window = NULL;

extern int option_print_ir;   /* gboolean (gint) défini dans le cœur */

void sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)
{
    (void)argc; (void)argv; (void)print_ir;
    /* ncurses (initscr) est ouvert dans render_loop, jamais en --print-ir. */
}

/* ─── ABI neutre qt6_* sur l'arbre WidgetNode ──────────────────────────────── */
void sermo_be_widget_show(void *w)
{ WidgetNode *n = (WidgetNode *)w; if (n) n->visible = TRUE; }

void sermo_be_widget_hide(void *w)
{ WidgetNode *n = (WidgetNode *)w; if (n) n->visible = FALSE; }

void sermo_be_widget_set_sensitive(void *w, int sensitive)
{ WidgetNode *n = (WidgetNode *)w; if (n) n->sensitive = (sensitive != 0); }

void sermo_be_widget_redraw(void *w)
{ (void)w; /* redessiné à chaque tour de boucle */ }

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
{ (void)w; (void)x; (void)y; /* placement géré par le terminal */ }

void *sermo_be_scroll_new(int w, int h)
{
    WidgetNode *n = widget_node_new(WT_VBOX, NULL, NULL);
    if (n) { n->width = w; n->height = h; }
    return n;
}

/* ─── Cycle de vie « application » ─────────────────────────────────────────── */
void sermo_be_app_init(int *argc, char ***argv)
{ (void)argc; (void)argv; }

int sermo_be_run_loop(void)
{
    if (option_print_ir) return 0;         /* headless : le cœur imprime l'IR */
    WidgetNode *root = sdl3_root_window;
    if (!root) return 0;

    render_loop(root,
        (root->label && *root->label) ? root->label : "ncursessermo",
        root->width, root->height);

    /* render_loop headless/interactif imprime déjà l'export (EXIT="abort" ou,
     * sur action exit:, le cœur a exporté et quitté). Rien d'autre à faire. */
    fflush(stdout);
    exit(EXIT_SUCCESS);
}

void sermo_be_app_quit(void)
{ /* la boucle sort sur 'q'/Esc ou action exit: */ }
