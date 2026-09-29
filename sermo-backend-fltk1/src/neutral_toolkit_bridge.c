/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* neutral_toolkit_bridge.c — pont ABI « toolkit neutre » du backend fltk1.
 *
 * libsermocore (variante NEUTRE) est compilé avec sermocore-shim.h,
 * qui EST qt6-compat.h. Ses macros de remplacement mappent les primitives
 * toolkit (gtk_widget_show, gtk_main, …) vers des fonctions préfixées « qt6_ »
 * — c'est le NOM de l'ABI neutre cœur→backend, indépendamment du toolkit réel.
 * Le cœur laisse donc ces 8 symboles indéfinis ; chaque backend neutre les
 * fournit dans son toolkit. Ce port réutilise les ponts C que fltk-compat.cpp
 * expose déjà (fltk_widget_show, fltk_group_add, …), qui portent la sémantique
 * FLTK EXACTE, testée, du port autonome. On ne fait donc que renommer.
 *
 * (fltk-compat.h est force-inclus sur cette unité via -include ; il déclare les
 *  fonctions fltk_* utilisées ci-dessous — Fl_Widget_C est un void.)
 */

/* Ponts FLTK C-appelables, définis dans fltk-compat.cpp. */
void  fltk_widget_show(void *w);
void  fltk_widget_hide(void *w);
void  fltk_widget_set_sensitive(void *w, int sensitive);
void  fltk_group_add(void *group, void *child);
void *fltk_group_child0(void *group);
void  fltk_window_move(void *w, int x, int y);
void *fltk_scroll_new(int w, int h);
void  fltk_run(void);
int   fltk_render_png(const char *file);   /* rendu offscreen PNG (fltk-compat.cpp) */

/* --render-png FILE : rendu offscreen déterministe, défini dans le cœur
 * (libsermocore/src/gtkdialog.c). En mode rendu on ne lance PAS la boucle. */
extern char *option_render_png;

/* ─── ABI neutre attendue par le cœur (noms qt6_*) → FLTK ─────────────────── */
void  sermo_be_widget_show(void *w)                    { fltk_widget_show(w); }
void  sermo_be_widget_hide(void *w)                    { fltk_widget_hide(w); }
void  sermo_be_widget_set_sensitive(void *w, int s)    { fltk_widget_set_sensitive(w, s); }
void  sermo_be_container_add(void *c, void *child)     { fltk_group_add(c, child); }
void *sermo_be_container_child0(void *c)               { return fltk_group_child0(c); }
void  sermo_be_window_move(void *w, int x, int y)      { fltk_window_move(w, x, y); }
void *sermo_be_scroll_new(int w, int h)                { return fltk_scroll_new(w, h); }

/* gtk_main() → sermo_be_run_loop() ; fltk_run() rend void, l'ABI attend un int.
 *
 * Mode rendu offscreen (--render-png FILE) : on dessine la fenêtre dans un PNG
 * et on sort SANS lancer la boucle d'événements — pendant FLTK du chemin qt6. */
int   sermo_be_run_loop(void)
{
    if (option_render_png && *option_render_png) {
        fltk_render_png(option_render_png);
        return 0;
    }
    fltk_run();
    return 0;
}
