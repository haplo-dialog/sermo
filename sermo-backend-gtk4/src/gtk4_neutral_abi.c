/* gtk4_neutral_abi.c — pont ABI neutre cœur→backend, implémenté en GTK4.
 * Le cœur (libsermocore variante NEUTRE) est compilé avec le shim : ses
 * opérations widget (gtk_widget_show, gtk_container_add, gtk_main, …) sont
 * mappées vers 8 fonctions qt6_* que CHAQUE backend neutre doit fournir. Ici
 * on les rend en GTK4, en réutilisant les shims GTK3→GTK4 de gtk4-compat.h
 * (force-inclus par le CMake : gtk_container_add, gtk_main → GMainLoop).
 * Les pointeurs de widgets sont des GtkWidget* que le cœur a reçus de nos
 * widget_*_create et nous rend tels quels.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include <gtk/gtk.h>

void qt6_widget_show(void *w)                 { if (w) gtk_widget_set_visible((GtkWidget *)w, TRUE); }
void qt6_widget_hide(void *w)                 { if (w) gtk_widget_set_visible((GtkWidget *)w, FALSE); }
void qt6_widget_set_sensitive(void *w, int s) { if (w) gtk_widget_set_sensitive((GtkWidget *)w, s ? TRUE : FALSE); }
void qt6_widget_redraw(void *w)               { if (w) gtk_widget_queue_draw((GtkWidget *)w); }

/* gtk_container_add est un shim gtk4-compat.h (per-widget child setter GTK4). */
void qt6_container_add(void *c, void *w)      { if (c && w) gtk_container_add((GtkWidget *)c, (GtkWidget *)w); }

/* gtk_bin_get_child : GTK4 = premier enfant. */
void *qt6_container_child0(void *c)           { return c ? gtk_widget_get_first_child((GtkWidget *)c) : NULL; }

/* GTK4 : la position est décidée par le WM (gtk_window_move retiré) → no-op. */
void qt6_window_move(void *w, int x, int y)   { (void)w; (void)x; (void)y; }

void *qt6_scroll_new(int w, int h)            { (void)w; (void)h; return gtk_scrolled_window_new(); }

/* gtk_main est un shim gtk4-compat.h (GMainLoop). */
int qt6_app_run(void)                         { gtk_main(); return 0; }
