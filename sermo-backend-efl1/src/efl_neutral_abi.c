/* efl_neutral_abi.c — implémentation EFL de l'ABI neutre cœur→backend.
 *
 * Le cœur (libsermocore, variante NEUTRE) est compilé avec sermocore-shim.h :
 * ses opérations widget (gtk_widget_show, gtk_container_add, gtk_main, …) y sont
 * mappées vers un petit jeu de fonctions « qt6_* » que CHAQUE backend neutre
 * doit fournir (le backend qt6 les implémente en Qt ; ici on les implémente en
 * EFL/Elementary). Les pointeurs de widgets sont des Evas_Object* que le cœur
 * a reçus des widget_*_create de ce backend et nous rend tels quels.
 *
 * Les cibles EFL reprennent exactement les mappings gtk_*→EFL de efl-compat.h
 * (evas_object_show/hide/move, elm_object_disabled_set, efl_container_add,
 * efl_scrolled_new, elm_run).
 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#include "efl-compat.h"   /* Evas/Elementary + helpers efl_container_add / efl_scrolled_new */
#include <Ecore_Evas.h>   /* moteur buffer : ecore_evas_manual_render, *_buffer_pixels_get */
#include "efl-globals.h"  /* g_efl_main_win */

/* efl_container_add et efl_scrolled_new sont des static inline de efl-compat.h. */

void sermo_be_widget_show(void *w)                 { if (w) evas_object_show((Evas_Object *)w); }
void sermo_be_widget_hide(void *w)                 { if (w) evas_object_hide((Evas_Object *)w); }
void sermo_be_widget_set_sensitive(void *w, int s) { if (w) elm_object_disabled_set((Evas_Object *)w, !s); }
void sermo_be_widget_redraw(void *w)               { (void)w; /* EFL redessine seul */ }

void sermo_be_container_add(void *c, void *w)
{
    if (c && w) efl_container_add((Evas_Object *)c, (Evas_Object *)w);
}

/* gtk_bin_get_child sous EFL renvoie le widget lui-même (efl-compat.h). */
void *sermo_be_container_child0(void *c) { return c; }

void sermo_be_window_move(void *w, int x, int y) { if (w) evas_object_move((Evas_Object *)w, x, y); }

void *sermo_be_scroll_new(int w, int h) { (void)w; (void)h; return efl_scrolled_new(); }

/* --render-png FILE : rendu offscreen déterministe (option parsée par le cœur,
 * gtkdialog.c). sermo_backend_toolkit_init() a forcé le moteur ecore_evas
 * « buffer » quand l'option est posée : la fenêtre vit sur un canvas mémoire. */
extern char *option_render_png;

/* Dessine la fenêtre principale dans un buffer ARGB et l'écrit en PNG, sans
 * boucle d'événements ni affichage. Rend 0 si le PNG a été sauvé, -1 sinon
 * (l'appelant se rabat alors sur la boucle / un autre chemin). */
static int efl_render_png(const char *path)
{
    Evas_Object *win = g_efl_main_win;
    if (!win) return -1;

    /* La fenêtre est déjà dimensionnée/affichée par widget_window_create.
     * On force un calcul complet du canvas puis le rendu manuel du buffer. */
    evas_object_show(win);
    Evas *e = evas_object_evas_get(win);
    if (!e) return -1;
    evas_smart_objects_calculate(e);

    Ecore_Evas *ee = ecore_evas_ecore_evas_get(e);
    if (!ee) return -1;

    /* Laisser Elementary exécuter ses jobs de layout différés, puis rendre. */
    int i;
    for (i = 0; i < 4; i++) {
        ecore_main_loop_iterate();
        evas_smart_objects_calculate(e);
    }
    ecore_evas_manual_render(ee);

    /* Pixels du moteur buffer : ARGB32 prémultiplié, w*h*4 octets. */
    int w = 0, h = 0;
    evas_output_size_get(e, &w, &h);
    const void *pixels = ecore_evas_buffer_pixels_get(ee);
    if (!pixels || w <= 0 || h <= 0) return -1;

    /* Recopier les pixels dans un objet image et laisser Evas sérialiser en
     * PNG (l'encodeur PNG est compilé dans libevas ; pas de dépendance externe
     * ni de capture « import »). */
    Evas_Object *img = evas_object_image_add(e);
    evas_object_image_colorspace_set(img, EVAS_COLORSPACE_ARGB8888);
    evas_object_image_size_set(img, w, h);
    evas_object_image_alpha_set(img, EINA_TRUE);
    evas_object_image_data_copy_set(img, (void *)pixels);
    evas_object_image_data_update_add(img, 0, 0, w, h);

    int ok = evas_object_image_save(img, path, NULL, "compress=9");
    evas_object_del(img);
    return ok ? 0 : -1;
}

/* Boucle d'événements (= gtk_main). elm_run() est void ; on rend 0. */
int sermo_be_run_loop(void)
{
    if (option_render_png && *option_render_png) {
        (void)efl_render_png(option_render_png);   /* sort sans boucle */
        return 0;
    }
    elm_run();
    return 0;
}
