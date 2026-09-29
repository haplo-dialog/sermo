/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* sermo_backend_sdl3sermo.c — hook d'init toolkit + adaptateur d'ABI neutre du
 * backend sdl3.
 *
 * Le coeur (libsermocore, variante NEUTRE) pilote parse/automaton/variables/
 * actions et assemble l'arbre de widgets en appelant une ABI « neutre » nommee
 * qt6_* (heritee du premier port neutre). Ce fichier realise cette ABI sur le
 * modele immediate-mode du port sdl3 : chaque widget est un WidgetNode, et
 * l'assemblage/visibilite se traduit en operations sur cet arbre. gtk_main
 * (= sermo_be_run_loop) entre dans la boucle SDL3/ImGui (render_loop).
 *
 * En --print-ir : aucun affichage (SDL_Init se fait dans render_loop, jamais
 * appelee dans ce mode).
 */
#include <stdio.h>
#include <stdlib.h>

#include "sermo-contract.h"   /* contrat cœur <-> backend (MIT) */
#include "dialog_state.h"   /* WidgetNode, widget_node_add_child, WT_* */
#include "render.h"         /* render_loop() */

/* ─── Globales que le coeur declare extern (visibilite des widgets) ────────
 * Definies jadis dans gtkdialog.c (desormais dans la lib) : le backend les
 * porte, comme il porte widget_*_create. GList vient du shim du port. */
GList *widget_show_list = NULL;
GList *widget_hide_list = NULL;

/* Racine capturee a la creation de la fenetre (widget_window_create), lue par
 * sermo_be_run_loop : le coeur ne passe plus la fenetre a gtk_main. */
WidgetNode *sdl3_root_window = NULL;

/* gboolean (gint) defini dans le coeur (gtkdialog.c). */
extern int option_print_ir;
/* --render-png FILE : rendu offscreen deterministe (gtkdialog.c). */
extern char *option_render_png;

/* ─── Hook d'init toolkit (carve 1.2) ─────────────────────────────────────── */
void sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)
{
    (void)argc;
    (void)argv;
    (void)print_ir;
    /* Rien a faire ici : SDL_Init + creation de fenetre sont dans render_loop,
     * qui n'est appelee qu'en mode graphique (pas en --print-ir). */
}

/* ─── ABI neutre qt6_* realisee sur l'arbre WidgetNode ─────────────────────── */

void sermo_be_widget_show(void *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (n) n->visible = TRUE;
}

void sermo_be_widget_hide(void *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (n) n->visible = FALSE;
}

void sermo_be_widget_set_sensitive(void *w, int sensitive)
{
    WidgetNode *n = (WidgetNode *)w;
    if (n) n->sensitive = (sensitive != 0);
}

void sermo_be_widget_redraw(void *w)
{
    (void)w;   /* immediate-mode : redessine a chaque frame, rien a marquer */
}

void sermo_be_container_add(void *container, void *child)
{
    WidgetNode *c  = (WidgetNode *)container;
    WidgetNode *ch = (WidgetNode *)child;
    if (c && ch)
        widget_node_add_child(c, ch);
}

void *sermo_be_container_child0(void *container)
{
    WidgetNode *c = (WidgetNode *)container;
    if (c && c->child_count > 0)
        return c->children[0];
    return NULL;
}

void sermo_be_window_move(void *w, int x, int y)
{
    (void)w; (void)x; (void)y;   /* placement gere par le WM/SDL, sans objet */
}

void *sermo_be_scroll_new(int w, int h)
{
    /* Conteneur de defilement : passe-plat vertical en immediate-mode.
     * L'enfant confie par sermo_be_container_add s'y empile ; ImGui gere lui-meme
     * le defilement de la fenetre. */
    WidgetNode *n = widget_node_new(WT_VBOX, NULL, NULL);
    if (n) {
        n->width  = w;
        n->height = h;
    }
    return n;
}

/* ─── Cycle de vie « application » (= gtk_init/gtk_main/gtk_main_quit) ─────── */

void sermo_be_app_init(int *argc, char ***argv)
{
    (void)argc; (void)argv;   /* SDL_Init differe a render_loop */
}

int sermo_be_run_loop(void)
{
    if (option_print_ir)
        return 0;                     /* headless : le coeur imprime l'IR */

    WidgetNode *root = sdl3_root_window;
    if (!root)
        return 0;

    /* Rendu offscreen : dessine une frame dans un framebuffer cache, ecrit le
     * PNG et sort proprement — PAS de boucle d'evenements, PAS de EXIT="abort"
     * (l'IA « voit » le dialogue sans capture externe). */
    if (option_render_png && *option_render_png) {
        int rc = render_loop(root,
            (root->label && *root->label) ? root->label : "sdl3sermo",
            root->width, root->height);
        fflush(stdout);
        exit(rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    render_loop(root,
        (root->label && *root->label) ? root->label : "sdl3sermo",
        root->width, root->height);

    /* Meme sortie que le port d'origine : la boucle est sortie sans action de
     * sortie (fenetre fermee) ; rendre la main au parser lui ferait imprimer
     * un faux « syntax error » sur le dernier token. */
    printf("EXIT=\"abort\"\n");
    fflush(stdout);
    exit(EXIT_SUCCESS);
}

void sermo_be_app_quit(void)
{
    /* La boucle SDL3 s'arrete sur evenement de fermeture ; rien a signaler. */
}
