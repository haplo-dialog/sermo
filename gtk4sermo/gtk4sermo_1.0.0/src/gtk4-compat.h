/*
 * gtk4-compat.h — GTK3→GTK4 compatibility shims for gtk4sermo
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026  Haplo-Linux <devel@haplo-dialog.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Strategy: provide drop-in macro/inline replacements for GTK3 APIs
 * removed or deprecated in GTK4, so the bulk of the codebase compiles
 * without touching every call site.
 *
 * This file is force-included by AM_CFLAGS (-include gtk4-compat.h)
 * so it applies to every .c file automatically.
 *
 * Coverage summary:
 *   ✅ gtk_container_add / remove / get_children / foreach
 *   ✅ gtk_box_pack_start / gtk_box_pack_end
 *   ✅ gtk_widget_show_all
 *   ✅ gtk_scrolled_window_add_with_viewport
 *   ✅ gtk_main / gtk_main_quit  (→ GMainLoop)
 *   ✅ gtk_entry_get_text / gtk_entry_set_text
 *   ✅ gtk_widget_destroy  (window vs widget)
 *   ✅ gtk_widget_get_toplevel
 *   ✅ gtk_widget_override_background_color/color/font  (stubbed → CSS TODO)
 *   ✅ gtk_widget_modify_bg/fg/font  (stubbed → CSS TODO)
 *   ✅ gtk_dialog_run  (stubbed → async TODO)
 *   ✅ gtk_file_chooser_get/set_filename
 *   ✅ GtkEventBox  (→ GtkBox stub)
 *   ✅ gtk_widget_size_request  (removed)
 *   ✅ GdkScreen  (→ GdkDisplay)
 *   ✅ gtk_image_set_from_stock (→ set_from_icon_name)
 *   ⚠️ GtkMenu / GtkMenuBar / GtkMenuItem  (removed — see widget_menu*.c stubs)
 *
 * Haplo-Linux <devel@haplo-dialog.fr> — 2026
 * License: GPL-2.0-or-later
 */

#ifndef GTK4_COMPAT_H
#define GTK4_COMPAT_H

/* Force-included first (-include): define feature-test macros before any
 * system header so putenv/strcasecmp/strncasecmp are declared even with
 * -D_POSIX_C_SOURCE=200809L. */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <stdlib.h>          /* putenv */
#include <string.h>
#include <strings.h>         /* strcasecmp, strncasecmp */

#include <gtk/gtk.h>
#include <glib/gprintf.h>    /* g_printf */

/* ================================================================
 * GMainLoop — backing store for gtk_main / gtk_main_quit shims.
 * Defined in gtk4-compat.c; extern here so all TUs share one loop.
 * ================================================================ */
extern GMainLoop *_gtk4sermo_main_loop;

/* ================================================================
 * gtk_main / gtk_main_quit  — removed in GTK4
 * ================================================================ */

static inline void _compat_gtk_main(void)
{
    _gtk4sermo_main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(_gtk4sermo_main_loop);
    g_main_loop_unref(_gtk4sermo_main_loop);
    _gtk4sermo_main_loop = NULL;
}

static inline void _compat_gtk_main_quit(void)
{
    if (_gtk4sermo_main_loop)
        g_main_loop_quit(_gtk4sermo_main_loop);
}

#define gtk_main()      _compat_gtk_main()
#define gtk_main_quit() _compat_gtk_main_quit()

/* ================================================================
 * GtkContainer  — removed in GTK4
 *   gtk_container_add        → per-widget child setter
 *   gtk_container_remove     → per-widget remove or gtk_widget_unparent
 *   gtk_container_get_children → GtkWidget first/next sibling iteration
 *   gtk_container_foreach    → same iteration
 * ================================================================ */

static inline void
_compat_container_add(GtkWidget *parent, GtkWidget *child)
{
    if (GTK_IS_BOX(parent))
        gtk_box_append(GTK_BOX(parent), child);
    else if (GTK_IS_SCROLLED_WINDOW(parent))
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(parent), child);
    else if (GTK_IS_FRAME(parent))
        gtk_frame_set_child(GTK_FRAME(parent), child);
    else if (GTK_IS_WINDOW(parent))
        gtk_window_set_child(GTK_WINDOW(parent), child);
    else if (GTK_IS_EXPANDER(parent))
        gtk_expander_set_child(GTK_EXPANDER(parent), child);
    else if (GTK_IS_VIEWPORT(parent))
        gtk_viewport_set_child(GTK_VIEWPORT(parent), child);
    else if (GTK_IS_LIST_BOX_ROW(parent))
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(parent), child);
    else if (GTK_IS_BUTTON(parent))
        gtk_button_set_child(GTK_BUTTON(parent), child);
    else if (GTK_IS_LIST_BOX(parent))
        gtk_list_box_append(GTK_LIST_BOX(parent), child);
    else if (GTK_IS_NOTEBOOK(parent))
        gtk_notebook_append_page(GTK_NOTEBOOK(parent), child, NULL);
    else if (GTK_IS_PANED(parent)) {
        if (gtk_paned_get_start_child(GTK_PANED(parent)) == NULL)
            gtk_paned_set_start_child(GTK_PANED(parent), child);
        else
            gtk_paned_set_end_child(GTK_PANED(parent), child);
    } else {
        g_warning("gtk4-compat: gtk_container_add: unhandled parent type '%s' — "
                  "child '%s' not added. GTK4_TODO: add explicit call.",
                  G_OBJECT_TYPE_NAME(parent), G_OBJECT_TYPE_NAME(child));
    }
}

#define gtk_container_add(parent, child) \
    _compat_container_add(GTK_WIDGET(parent), GTK_WIDGET(child))

static inline void
_compat_container_remove(GtkWidget *parent, GtkWidget *child)
{
    if (GTK_IS_BOX(parent))
        gtk_box_remove(GTK_BOX(parent), child);
    else if (GTK_IS_SCROLLED_WINDOW(parent))
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(parent), NULL);
    else if (GTK_IS_LIST_BOX(parent))
        gtk_list_box_remove(GTK_LIST_BOX(parent), child);
    else if (GTK_IS_NOTEBOOK(parent)) {
        gint n = gtk_notebook_page_num(GTK_NOTEBOOK(parent), child);
        if (n >= 0) gtk_notebook_remove_page(GTK_NOTEBOOK(parent), n);
    } else {
        gtk_widget_unparent(child);
    }
}

#define gtk_container_remove(parent, child) \
    _compat_container_remove(GTK_WIDGET(parent), GTK_WIDGET(child))

static inline GList *
_compat_container_get_children(GtkWidget *parent)
{
    GList    *list  = NULL;
    GtkWidget *child = gtk_widget_get_first_child(parent);
    while (child) {
        list  = g_list_append(list, child);
        child = gtk_widget_get_next_sibling(child);
    }
    return list;
}

#define gtk_container_get_children(w) \
    _compat_container_get_children(GTK_WIDGET(w))

/* gtk_container_foreach: iterate with a GtkCallback */
#define gtk_container_foreach(parent, func, data)                       \
    do {                                                                 \
        GtkWidget *_c = gtk_widget_get_first_child(GTK_WIDGET(parent)); \
        while (_c) {                                                     \
            GtkWidget *_n = gtk_widget_get_next_sibling(_c);            \
            ((GtkCallback)(func))(_c, (data));                          \
            _c = _n;                                                     \
        }                                                                \
    } while (0)

/* ================================================================
 * gtk_box_pack_start / gtk_box_pack_end  — removed in GTK4
 *   → gtk_box_append (both: GTK4 has no pack-end in the same sense)
 *   expand/fill/padding mapped to hexpand + margins
 * ================================================================ */

static inline void
_compat_box_pack(GtkBox *box, GtkWidget *child,
                 gboolean expand, gboolean fill, guint padding)
{
    (void)fill; /* no direct GTK4 equivalent */
    gtk_box_append(box, child);
    if (expand) {
        gtk_widget_set_hexpand(child, TRUE);
        gtk_widget_set_vexpand(child, TRUE);
    }
    if (padding > 0) {
        gtk_widget_set_margin_start (child, (int)padding);
        gtk_widget_set_margin_end   (child, (int)padding);
        gtk_widget_set_margin_top   (child, (int)padding);
        gtk_widget_set_margin_bottom(child, (int)padding);
    }
}

#define gtk_box_pack_start(box, child, expand, fill, padding) \
    _compat_box_pack(GTK_BOX(box), GTK_WIDGET(child), (expand), (fill), (padding))

#define gtk_box_pack_end(box, child, expand, fill, padding) \
    _compat_box_pack(GTK_BOX(box), GTK_WIDGET(child), (expand), (fill), (padding))

/* ================================================================
 * gtk_widget_show_all  — removed in GTK4
 * Widgets are visible by default; just show the root.
 * ================================================================ */

#define gtk_widget_show_all(w) gtk_widget_show(GTK_WIDGET(w))

/* ================================================================
 * gtk_scrolled_window_add_with_viewport  — removed in GTK4
 * GtkScrolledWindow auto-wraps in a viewport when needed.
 * ================================================================ */

#define gtk_scrolled_window_add_with_viewport(sw, child) \
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), GTK_WIDGET(child))

/* ================================================================
 * GtkEntry text accessors — deprecated in GTK4
 *   gtk_entry_get_text → gtk_editable_get_text
 *   gtk_entry_set_text → gtk_editable_set_text
 * ================================================================ */

#define gtk_entry_get_text(e)       gtk_editable_get_text(GTK_EDITABLE(e))
#define gtk_entry_set_text(e, t)    gtk_editable_set_text(GTK_EDITABLE(e), (t))

/* ================================================================
 * gtk_widget_destroy  — semantics changed in GTK4
 *   Windows: gtk_window_destroy()
 *   Others:  gtk_widget_unparent()
 * ================================================================ */

static inline void _compat_widget_destroy(GtkWidget *w)
{
    if (GTK_IS_WINDOW(w))
        gtk_window_destroy(GTK_WINDOW(w));
    else
        gtk_widget_unparent(w);
}

#define gtk_widget_destroy(w) _compat_widget_destroy(GTK_WIDGET(w))

/* ================================================================
 * gtk_init  — in GTK4 takes no arguments (was gtk_init(&argc,&argv))
 * ================================================================ */

#define gtk_init(argc, argv) ((void)(argc), (void)(argv), (gtk_init)())
/* gtk_init_check — GTK4 prend aussi zéro argument et renvoie gboolean ;
 * non fatal sans serveur X (utilisé en mode --print-ir, comme gtk3sermo). */
#define gtk_init_check(argc, argv) ((void)(argc), (void)(argv), (gtk_init_check)())

/* ================================================================
 * gtk_widget_get_toplevel  — removed in GTK4
 *   → gtk_widget_get_root (returns GtkRoot, cast to GtkWidget)
 * ================================================================ */

#define gtk_widget_get_toplevel(w) \
    GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(w)))

/* ================================================================
 * gtk_widget_override_* / gtk_widget_modify_*
 * Completely removed in GTK4. Must use CSS providers.
 * Stubbed here to allow compilation.
 * GTK4_TODO: replace each call site with gtk_widget_add_css_class()
 *            or gtk_css_provider_load_from_string().
 * ================================================================ */

/* ================================================================
 * gtk_widget_override_* / gtk_widget_modify_*  — CSS implementations v2
 *
 * Remplacent les no-ops par de vraies injections CSS via GtkCssProvider.
 * Chaque widget reçoit un GtkCssProvider unique attaché à son GdkDisplay.
 *
 * Limitations connues :
 *   - GtkStateFlags ignoré (GTK4 stylise via pseudo-classes CSS :hover etc.)
 *   - PangoFontDescription→CSS : conversion approximative (famille + taille)
 * ================================================================ */

/* Helper interne — injecte une règle CSS sur un widget donné */
static inline void
_compat_css_inject(GtkWidget *w, const gchar *css)
{
    GtkCssProvider *prov = gtk_css_provider_new();
#if GTK_CHECK_VERSION(4, 12, 0)
    gtk_css_provider_load_from_string(prov, css);
#else
    gtk_css_provider_load_from_data(prov, css, -1);
#endif
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(w),
        GTK_STYLE_PROVIDER(prov),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(prov);
}

/* Helper — GdkRGBA → chaîne "rgba(r,g,b,a)" */
static inline gchar *
_compat_rgba_to_css(const GdkRGBA *c)
{
    if (!c) return g_strdup("inherit");
    return g_strdup_printf("rgba(%d,%d,%d,%.3f)",
        (int)(c->red   * 255),
        (int)(c->green * 255),
        (int)(c->blue  * 255),
        c->alpha);
}

/* Helper — PangoFontDescription → fragment CSS font */
static inline gchar *
_compat_pango_to_css_font(const PangoFontDescription *fd)
{
    if (!fd) return g_strdup("");
    const char *family = pango_font_description_get_family(fd);
    int size_pt = pango_font_description_get_size(fd) / PANGO_SCALE;
    if (size_pt <= 0) size_pt = 10;
    return g_strdup_printf("font-family: \"%s\"; font-size: %dpt;",
                           family ? family : "sans", size_pt);
}

/* Arrière-plan */
static inline void
_compat_override_bg(GtkWidget *w, int state, const GdkRGBA *c)
{
    (void)state;
    gchar *col = _compat_rgba_to_css(c);
    gchar *css = g_strdup_printf("* { background-color: %s; }", col);
    _compat_css_inject(w, css);
    g_free(col); g_free(css);
}

/* Couleur du texte */
static inline void
_compat_override_color(GtkWidget *w, int state, const GdkRGBA *c)
{
    (void)state;
    gchar *col = _compat_rgba_to_css(c);
    gchar *css = g_strdup_printf("* { color: %s; }", col);
    _compat_css_inject(w, css);
    g_free(col); g_free(css);
}

/* Police */
static inline void
_compat_override_font(GtkWidget *w, const PangoFontDescription *fd)
{
    gchar *font_css = _compat_pango_to_css_font(fd);
    gchar *css = g_strdup_printf("* { %s }", font_css);
    _compat_css_inject(w, css);
    g_free(font_css); g_free(css);
}

#define gtk_widget_override_background_color(w, s, c)     _compat_override_bg(GTK_WIDGET(w), (s), (c))
#define gtk_widget_override_color(w, s, c)     _compat_override_color(GTK_WIDGET(w), (s), (c))
#define gtk_widget_override_font(w, fd)     _compat_override_font(GTK_WIDGET(w), (fd))
#define gtk_widget_override_cursor(w, c, sc)     _compat_override_color(GTK_WIDGET(w), 0, (c))  /* approx */
#define gtk_widget_modify_bg(w, s, c)     _compat_override_bg(GTK_WIDGET(w), (s), (c))
#define gtk_widget_modify_fg(w, s, c)     _compat_override_color(GTK_WIDGET(w), (s), (c))
#define gtk_widget_modify_text(w, s, c)     _compat_override_color(GTK_WIDGET(w), (s), (c))
#define gtk_widget_modify_base(w, s, c)     _compat_override_bg(GTK_WIDGET(w), (s), (c))
#define gtk_widget_modify_font(w, fd)     _compat_override_font(GTK_WIDGET(w), (fd))

/* ================================================================
 * gtk_dialog_run  — removed in GTK4
 *
 * Émulation synchrone via GMainLoop local + signal "response".
 * Bloque l'appelant jusqu'à ce que l'utilisateur réponde,
 * sans bloquer le reste du main loop GTK (les événements continuent).
 *
 * Compatible avec GtkDialog et ses sous-classes (GtkFileChooserDialog,
 * GtkFontChooserDialog, GtkColorChooserDialog).
 *
 * Note : GTK4 encourage les APIs async (GtkFileDialog etc.) mais cette
 * émulation reste valide pour les boîtes de dialogue modales simples.
 * ================================================================ */

typedef struct {
    GMainLoop *loop;
    gint       response;
} _Gtk4DialogRunData;

static inline void
_compat_dialog_response_cb(GtkDialog *dialog, gint response_id, gpointer data)
{
    _Gtk4DialogRunData *rd = (_Gtk4DialogRunData *)data;
    rd->response = response_id;
    if (g_main_loop_is_running(rd->loop))
        g_main_loop_quit(rd->loop);
}

static inline gint
_compat_gtk_dialog_run(GtkDialog *dialog)
{
    _Gtk4DialogRunData rd;
    rd.loop     = g_main_loop_new(NULL, FALSE);
    rd.response = GTK_RESPONSE_DELETE_EVENT;

    gulong sig = g_signal_connect(dialog, "response",
                                  G_CALLBACK(_compat_dialog_response_cb), &rd);
    gtk_widget_show(GTK_WIDGET(dialog));
    g_main_loop_run(rd.loop);
    g_signal_handler_disconnect(dialog, sig);
    g_main_loop_unref(rd.loop);
    return rd.response;
}

#define gtk_dialog_run(d) _compat_gtk_dialog_run(GTK_DIALOG(d))

/* ================================================================
 * GtkFileChooser filename API  — deprecated in GTK4
 *   gtk_file_chooser_get_filename → get_file() + g_file_get_path()
 *   gtk_file_chooser_set_filename → set_file()
 * ================================================================ */

static inline gchar *
_compat_fc_get_filename(GtkFileChooser *fc)
{
    GFile *f = gtk_file_chooser_get_file(fc);
    if (!f) return NULL;
    gchar *path = g_file_get_path(f);
    g_object_unref(f);
    return path;
}

static inline void
_compat_fc_set_filename(GtkFileChooser *fc, const gchar *path)
{
    if (!path) return;
    GFile *f = g_file_new_for_path(path);
    gtk_file_chooser_set_file(fc, f, NULL);
    g_object_unref(f);
}

#define gtk_file_chooser_get_filename(fc) \
    _compat_fc_get_filename(GTK_FILE_CHOOSER(fc))
#define gtk_file_chooser_set_filename(fc, fn) \
    _compat_fc_set_filename(GTK_FILE_CHOOSER(fc), (fn))

/* GTK4 removed the CREATE_FOLDER action; SELECT_FOLDER covers the use case. */
#ifndef GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
#define GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
#endif

/* ================================================================
 * GtkEventBox  — removed in GTK4
 * Stub as a plain GtkBox. Event handling must use GtkEventControllerKey
 * etc. directly on the parent widget.
 * GTK4_TODO: per call-site migration to event controllers.
 * ================================================================ */

#define gtk_event_box_new()         gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)
#define GTK_IS_EVENT_BOX(w)         GTK_IS_BOX(w)
#define GTK_EVENT_BOX(w)            GTK_BOX(w)

/* ================================================================
 * gtk_widget_size_request  — removed in GTK4
 * ================================================================ */

#define gtk_widget_size_request(w, req)  gtk_widget_get_preferred_size(w, req, NULL)

/* ================================================================
 * GdkScreen  — removed in GTK4, replaced by GdkDisplay
 * ================================================================ */

#ifndef GDK_DISABLE_DEPRECATION_WARNINGS
#  define GdkScreen          GdkDisplay
#  define gdk_screen_get_default()  gdk_display_get_default()
#  define gdk_screen_get_monitor_geometry(s, n, g) \
        ({ GdkMonitor *_m = gdk_display_get_monitor(s, n); \
           gdk_monitor_get_geometry(_m, g); })
#endif

/* ================================================================
 * gtk_image_set_from_stock / gtk_image_new_from_stock
 * Already migrated to icon_name in GTK3 port; kept as safety net.
 * ================================================================ */

#define gtk_image_new_from_stock(stock, size) \
    gtk_image_new_from_icon_name(stock)

/* ================================================================
 * gtk_widget_input_shape_combine_region — removed
 * ================================================================ */

#define gtk_widget_input_shape_combine_region(w, r) /* no-op GTK4 */

/* ================================================================
 * GtkComboBoxText — still present in GTK4 (up to 4.10+),
 * included for completeness. No shim needed yet.
 * GTK4_TODO (4.10+): migrate to GtkDropDown.
 * ================================================================ */

/* ================================================================
 * GtkTreeView / GtkListStore — deprecated in GTK4.10+ but compiles.
 * No shim needed. GTK4_TODO: migrate to GtkColumnView / GListModel.
 * ================================================================ */

/* ================================================================
 * GtkMenu / GtkMenuBar / GtkMenuItem  — COMPLETELY REMOVED in GTK4.
 * Remplacés par GMenuModel + GtkPopoverMenuBar + GtkPopoverMenu.
 * widget_menubar.c et widget_menuitem.c utilisent maintenant les
 * API GTK4 natives — GTK4_NO_MENU n'est plus défini.
 * ================================================================ */
/* GTK4_NO_MENU — supprimé en v2, widgets réels via GMenuModel */

/* ================================================================
 * gtk_window_set_default — renamed in GTK4
 * ================================================================ */

/* gtk_window_set_default still exists in GTK4 as
 * gtk_window_set_default_widget() */
#define gtk_window_set_default(w, widget) \
    gtk_window_set_default_widget(GTK_WINDOW(w), GTK_WIDGET(widget))

/* ================================================================
 * GtkWidget realize / unrealize signals — still present ✅
 * GtkWidget draw signal removed → use gtk_drawing_area_set_draw_func
 * ================================================================ */

/* ================================================================
 * gtk_widget_set_double_buffered — removed (always on in GTK4)
 * ================================================================ */

#define gtk_widget_set_double_buffered(w, v)  /* no-op GTK4 */

/* ================================================================
 * gtk_widget_set_app_paintable — removed in GTK4
 * ================================================================ */

#define gtk_widget_set_app_paintable(w, v) /* no-op GTK4 */

/* ================================================================
 * gtk_icon_theme_get_default  — deprecated in GTK4
 *   → gtk_icon_theme_get_for_display(gdk_display_get_default())
 * ================================================================ */

#define gtk_icon_theme_get_default() \
    gtk_icon_theme_get_for_display(gdk_display_get_default())

/* gtk_icon_theme_load_icon still exists in GTK4 ✅ */

/* ================================================================
 * gtk_main_iteration_do  — removed in GTK4
 *   → g_main_context_iteration
 * ================================================================ */

#define gtk_main_iteration_do(blocking) \
    g_main_context_iteration(NULL, (blocking))

/* gtk_events_pending → g_main_context_pending */
#define gtk_events_pending() \
    g_main_context_pending(NULL)

/* ================================================================
 * gdk_threads_enter / gdk_threads_leave  — removed in GTK4
 * GTK4 is single-threaded by design (all UI on main thread).
 * Stub as no-ops; caller must ensure GTK calls happen on main thread.
 * ================================================================ */

#define gdk_threads_enter()  /* GTK4: single-threaded, no-op */
#define gdk_threads_leave()  /* GTK4: single-threaded, no-op */

/* ================================================================
 * gdk_cairo_set_source_rgba — still works in GTK4 ✅
 * ================================================================ */

/* ================================================================
 * "draw" signal — removed in GTK4
 * In GTK4, GtkDrawingArea uses gtk_drawing_area_set_draw_func().
 * The signal name string "draw" that appears in signal tables will
 * silently fail to connect, but at least won't crash at link time.
 * GTK4_TODO: migrate widget_drawingarea to set_draw_func per call site.
 * ================================================================ */

/* ================================================================
 * GtkComboBox deprecated text API — removed in GTK4
 *   gtk_combo_box_new_text()     → gtk_combo_box_text_new()
 *   gtk_combo_box_append_text()  → gtk_combo_box_text_append_text()
 *   gtk_combo_box_insert_text()  → gtk_combo_box_text_insert_text()
 *   gtk_combo_box_remove_text()  → gtk_combo_box_text_remove()
 *   gtk_combo_box_get_active_text() → gtk_combo_box_text_get_active_text()
 * ================================================================ */

#define gtk_combo_box_new_text() \
    gtk_combo_box_text_new()

#define gtk_combo_box_append_text(cb, text) \
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb), (text))

#define gtk_combo_box_insert_text(cb, pos, text) \
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(cb), (pos), (text))

#define gtk_combo_box_remove_text(cb, pos) \
    gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(cb), (pos))

#define gtk_combo_box_get_active_text(cb) \
    gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cb))

/* ================================================================
 * GtkCList — GTK2 only, never existed in GTK3/4.
 * widget_table.c a été réécrit pour utiliser GtkColumnView + GListStore.
 * GTK4_NO_CLIST n'est plus nécessaire.
 * ================================================================ */
/* GTK4_NO_CLIST — supprimé en v2, widget_table utilise GtkColumnView */

/* ================================================================
 * GtkSocket / GtkPlug  — removed in GTK4 (was X11-only)
 * create_gvim() in automaton.c uses GtkSocket; stubbed out.
 * ================================================================ */

#define GTK4_NO_SOCKET 1

/* ================================================================
 * g_thread_init / gdk_threads_init  — removed in GLib 2.32 / GTK4
 * GTK4 initialises threading automatically; these are no-ops.
 * ================================================================ */

#define g_thread_init(vtable)  /* GTK4/GLib2.32+: automatic */
#define gdk_threads_init()     /* GTK4: no explicit init needed */

/* ================================================================
 * gtk_widget_set_extension_events  — removed in GTK4
 * Input extension events are handled differently (input devices API).
 * ================================================================ */

#define gtk_widget_set_extension_events(w, mask) /* GTK4 no-op */

/* ================================================================
 * gtk_font_button_get_font_name / set_font_name
 * Deprecated in GTK4 — replaced by GtkFontChooser interface.
 *   gtk_font_button_get_font_name → gtk_font_chooser_get_font
 *   gtk_font_button_set_font_name → gtk_font_chooser_set_font
 * ================================================================ */

#define gtk_font_button_get_font_name(fb) \
    gtk_font_chooser_get_font(GTK_FONT_CHOOSER(fb))

#define gtk_font_button_set_font_name(fb, name) \
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(fb), (name))

/* ================================================================
 * gtk_window_new — signature changed in GTK4
 * GTK3: gtk_window_new(GtkWindowType type)   (GTK_WINDOW_TOPLEVEL etc.)
 * GTK4: gtk_window_new(void)
 * ================================================================ */

#ifdef gtk_window_new
#undef gtk_window_new
#endif
#define gtk_window_new(type) (gtk_window_new)()

/* ================================================================
 * GtkWindowType enum — removed in GTK4
 * GTK_WINDOW_TOPLEVEL and GTK_WINDOW_POPUP were values of GtkWindowType.
 * In GTK4 all windows are toplevel; the distinction no longer exists.
 * Define as 0 so existing call sites compile (the type arg is ignored
 * by our gtk_window_new shim above).
 * ================================================================ */

#ifndef GTK_WINDOW_TOPLEVEL
#  define GTK_WINDOW_TOPLEVEL 0   /* GTK4: all windows are toplevel */
#endif
#ifndef GTK_WINDOW_POPUP
#  define GTK_WINDOW_POPUP    0   /* GTK4: use GtkPopover instead */
#endif

/* ================================================================
 * GtkColorButton alpha/use_alpha API — deprecated in GTK4 (4.10+)
 * In GTK4 use the GtkColorChooser interface directly.
 *   gtk_color_button_get_use_alpha → gtk_color_chooser_get_use_alpha
 *   gtk_color_button_set_use_alpha → gtk_color_chooser_set_use_alpha
 *   gtk_color_button_get_alpha     → read alpha from gtk_color_chooser_get_rgba
 *   gtk_color_button_set_alpha     → set via gtk_color_chooser_set_rgba
 * NOTE: these still exist in GTK4 < 4.10 but generate deprecation warnings.
 * The shims silence the warnings on GTK4 >= 4.10.
 * ================================================================ */

#define gtk_color_button_get_use_alpha(btn) \
    gtk_color_chooser_get_use_alpha(GTK_COLOR_CHOOSER(btn))

#define gtk_color_button_set_use_alpha(btn, val) \
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(btn), (val))

static inline guint16
_compat_color_button_get_alpha(GtkWidget *btn)
{
    GdkRGBA c = {0};
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(btn), &c);
    return (guint16)(c.alpha * 65535.0 + 0.5);
}

static inline void
_compat_color_button_set_alpha(GtkWidget *btn, guint16 alpha)
{
    GdkRGBA c = {0};
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(btn), &c);
    c.alpha = alpha / 65535.0;
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(btn), &c);
}

#define gtk_color_button_get_alpha(btn) \
    _compat_color_button_get_alpha(GTK_WIDGET(btn))

#define gtk_color_button_set_alpha(btn, alpha) \
    _compat_color_button_set_alpha(GTK_WIDGET(btn), (alpha))

/* ================================================================
 * GtkCalendar API — changed in GTK4
 *   gtk_calendar_get_date(cal, &year, &month, &day)
 *     → gtk_calendar_get_date(cal) returns GDateTime*
 *   gtk_calendar_select_month(cal, month, year)
 *     → gtk_calendar_select_month(cal, month, year) STILL EXISTS in GTK4 ✅
 *   gtk_calendar_select_day(cal, day)
 *     → gtk_calendar_select_day(cal, day) STILL EXISTS in GTK4 ✅
 * Only gtk_calendar_get_date() changed — shim here.
 * ================================================================ */

static inline void
_compat_calendar_get_date(GtkCalendar *cal,
                          guint *year, guint *month, guint *day)
{
    GDateTime *dt = gtk_calendar_get_date(cal);
    if (dt) {
        if (year)  *year  = (guint)g_date_time_get_year(dt);
        if (month) *month = (guint)g_date_time_get_month(dt) - 1; /* 0-based */
        if (day)   *day   = (guint)g_date_time_get_day_of_month(dt);
        g_date_time_unref(dt);
    } else {
        if (year)  *year  = 0;
        if (month) *month = 0;
        if (day)   *day   = 1;
    }
}

#define gtk_calendar_get_date(cal, year, month, day) \
    _compat_calendar_get_date(GTK_CALENDAR(cal), (year), (month), (day))

/* ================================================================
 * GtkInfoBar — removed in GTK4
 * Replaced by a GtkBox + GtkLabel styled with CSS.
 * We keep the API surface as a thin struct wrapper so widget_infobar.c
 * compiles unchanged. The infobar is rendered as a coloured GtkBox.
 * ================================================================ */

#define GTK4_NO_INFOBAR 1   /* signal to widget_infobar.c */

/* Map the GTK3 GtkInfoBar API to our GtkBox-based replacement */
#define gtk_info_bar_new()                   gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6)
#define gtk_info_bar_set_message_type(w, t)  /* GTK4_TODO: apply CSS class */
#define gtk_info_bar_set_show_close_button(w, v) /* no-op in GTK4 box */

static inline GtkWidget *
_compat_info_bar_get_content_area(GtkWidget *box)
{
    /* In our shim the "content area" is the box itself */
    return box;
}
#define gtk_info_bar_get_content_area(w) _compat_info_bar_get_content_area(GTK_WIDGET(w))
#undef GTK_INFO_BAR
#define GTK_INFO_BAR(w)  GTK_BOX(w)
#undef GTK_IS_INFO_BAR
#define GTK_IS_INFO_BAR(w) GTK_IS_BOX(w)

/* GtkMessageType : toujours fourni par GTK4 (gtkenums.h, utilisé par
 * GtkMessageDialog encore présent). Aucune redéfinition nécessaire —
 * une redéfinition entre d'ailleurs en conflit avec l'enum GTK4. */

/* ================================================================
 * GtkAspectFrame — constructor changed in GTK4
 *   GTK3: gtk_aspect_frame_new(label, xalign, yalign, ratio, obey_child)
 *   GTK4: gtk_aspect_frame_new(xalign, yalign, ratio, obey_child)
 *   (label is gone — use gtk_frame_set_label() separately if needed)
 * ================================================================ */

static inline GtkWidget *
_compat_aspect_frame_new(const gchar *label,
                         gfloat xalign, gfloat yalign,
                         gfloat ratio, gboolean obey_child)
{
    GtkWidget *af = gtk_aspect_frame_new(xalign, yalign, ratio, obey_child);
    if (label && *label)
        gtk_frame_set_label(GTK_FRAME(af), label);
    return af;
}

#define gtk_aspect_frame_new(label, xalign, yalign, ratio, obey_child) \
    _compat_aspect_frame_new((label), (xalign), (yalign), (ratio), (obey_child))

/* ================================================================
 * GtkFileChooserButton — removed in GTK4
 * Use GtkFileDialog (async) or keep a simple button + GtkFileChooserNative.
 * For compatibility: stub as GtkButton that shows a GtkFileChooserNative.
 * GTK4_TODO: full async migration per widget_filechooser.c call sites.
 * ================================================================ */

#define GTK4_NO_FILECHOOSERBUTTON 1

/* gtk_file_chooser_button_new shimmed to a plain GtkButton */
#define gtk_file_chooser_button_new(title, action) \
    gtk_button_new_with_label(title)

/* ================================================================
 * gtk_widget_hide — still present in GTK4 ✅
 * gtk_widget_show — still present in GTK4 ✅
 * ================================================================ */

/* ================================================================
 * GtkVScale / GtkHScale — removed in GTK4
 * → GtkScale with orientation
 * ================================================================ */

#define gtk_vscale_new(adj) \
    gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(adj))
#define gtk_vscale_new_with_range(min, max, step) \
    gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, (min), (max), (step))
#define gtk_hscale_new(adj) \
    gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT(adj))
#define gtk_hscale_new_with_range(min, max, step) \
    gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, (min), (max), (step))
#define GTK_VSCALE(w) GTK_SCALE(w)
#define GTK_HSCALE(w) GTK_SCALE(w)

/* ================================================================
 * gtk_toggle_button_new / gtk_toggle_button_get_active — GTK4 ✅
 * ================================================================ */

/* GtkToggleButton still exists in GTK4 — no shim needed */

/* ================================================================
 * gtk_label_new / gtk_label_set_text — GTK4 ✅
 * ================================================================ */

/* ================================================================
 * GtkBin / GTK_CONTAINER — removed in GTK4
 *   GtkBin était la classe de base des conteneurs à enfant unique.
 *   gtk_bin_get_child → gtk_widget_get_first_child
 *   GTK_BIN / GTK_CONTAINER → simple cast vers GtkWidget*
 * ================================================================ */

#define GTK_BIN(w)              GTK_WIDGET(w)
#define GTK_IS_BIN(w)           (GTK_IS_WIDGET(w))
#define gtk_bin_get_child(w)    gtk_widget_get_first_child(GTK_WIDGET(w))

/* GTK_CONTAINER : utilisé par gtk_container_add(GTK_CONTAINER(x), ...).
 * Nos shims gtk_container_* attendent un GtkWidget*, donc on laisse passer
 * le pointeur tel quel. */
#define GTK_CONTAINER(w)        (w)
#ifndef GTK_IS_CONTAINER
#  define GTK_IS_CONTAINER(w)   (GTK_IS_WIDGET(w))
#endif

/* ================================================================
 * gtk_scrolled_window_new — signature changée en GTK4
 *   GTK3 : gtk_scrolled_window_new(GtkAdjustment *h, GtkAdjustment *v)
 *   GTK4 : gtk_scrolled_window_new(void)
 * ================================================================ */

#ifdef gtk_scrolled_window_new
#  undef gtk_scrolled_window_new
#endif
#define gtk_scrolled_window_new(...)   (gtk_scrolled_window_new)()

/* ================================================================
 * GtkFileChooser : set_current_folder / add_shortcut_folder
 *   GTK4 prend des GFile* + GError**, plus des chemins char*.
 * ================================================================ */

static inline gboolean
_compat_fc_set_current_folder(GtkFileChooser *fc, const char *path)
{
    GFile *f = g_file_new_for_path(path);
    gboolean r = gtk_file_chooser_set_current_folder(fc, f, NULL);
    g_object_unref(f);
    return r;
}
#define gtk_file_chooser_set_current_folder(fc, path) \
    _compat_fc_set_current_folder(GTK_FILE_CHOOSER(fc), (path))

static inline gboolean
_compat_fc_add_shortcut_folder(GtkFileChooser *fc, const char *path, GError **err)
{
    GFile *f = g_file_new_for_path(path);
    gboolean r = gtk_file_chooser_add_shortcut_folder(fc, f, err);
    g_object_unref(f);
    return r;
}
#define gtk_file_chooser_add_shortcut_folder(fc, path, err) \
    _compat_fc_add_shortcut_folder(GTK_FILE_CHOOSER(fc), (path), (err))

/* ================================================================
 * GtkMenuItem / GtkCheckMenuItem / GtkRadioMenuItem — RETIRÉS en GTK4.
 * Les menus sont reconstruits via GMenuModel / GtkPopoverMenu.
 * Ces macros permettent aux branches héritées de compiler (code mort) ;
 * elles renvoient toujours FALSE car ces types n'existent plus.
 * ================================================================ */

#define GTK_IS_MENU_ITEM(w)              (FALSE)
#define GTK_IS_CHECK_MENU_ITEM(w)        (FALSE)
#define GTK_IS_RADIO_MENU_ITEM(w)        (FALSE)
#define GTK_CHECK_MENU_ITEM(w)           (w)
#define gtk_check_menu_item_get_active(w) (FALSE)

/* ================================================================
 * vte_terminal_get_padding — retiré dans vte-gtk4.
 *   On renvoie un padding nul (l'info n'est plus exposée).
 * ================================================================ */

#define vte_terminal_get_padding(t, xptr, yptr) \
    do { *(xptr) = 0; *(yptr) = 0; } while (0)

/* ================================================================
 * gtk_viewport_set_shadow_type — retiré en GTK4 (style via CSS).
 * ================================================================ */

#define gtk_viewport_set_shadow_type(vp, type)   /* no-op GTK4 */

/* ================================================================
 * gtk_widget_set_uposition — retiré (API GTK1/2). No-op en GTK4 :
 * le positionnement explicite des fenêtres n'est plus supporté.
 * ================================================================ */

#define gtk_widget_set_uposition(w, x, y)        do { (void)(x); (void)(y); } while (0)

/* ================================================================
 * GdkEvent* — les structures d'évènement concrètes ont été retirées
 * en GTK4 (GdkEvent est désormais opaque, accès via gdk_event_get_*).
 * Le cœur de gtkdialog déclare encore des gestionnaires typés
 * (GdkEventButton*, etc.). On fournit des structures de compatibilité
 * pour que ces signatures compilent. NOTE : en GTK4 ces handlers ne
 * reçoivent plus ces structures au runtime — migration par contrôleurs
 * d'évènements requise (GTK4_TODO).
 * ================================================================ */

#ifndef GDK_EXPOSE
#  define GDK_EXPOSE     2
#endif
#ifndef GDK_CONFIGURE
#  define GDK_CONFIGURE  13
#endif

typedef struct {
    int        type;
    GdkSurface *window;
    gint8      send_event;
    guint32    time;
    double     x, y;
    double    *axes;
    guint      state;
    guint      button;
    GdkDevice *device;
    double     x_root, y_root;
} GdkEventButton;

typedef struct {
    int        type;
    GdkSurface *window;
    gint8      send_event;
    int        x, y;
    int        width, height;
} GdkEventConfigure;

typedef struct {
    int        type;
    GdkSurface *window;
    gint8      send_event;
    GdkSurface *subwindow;
    guint32    time;
    double     x, y;
    double     x_root, y_root;
    guint      state;
    int        mode;
    int        detail;
    gboolean   focus;
} GdkEventCrossing;

typedef struct {
    int        type;
    GdkSurface *window;
    gint8      send_event;
    gint16     in;
} GdkEventFocus;

typedef struct {
    int        type;
    GdkSurface *window;
    gint8      send_event;
    guint32    time;
    guint      state;
    guint      keyval;
    int        length;
    char      *string;
    guint16    hardware_keycode;
    guint8     group;
    guint      is_modifier : 1;
} GdkEventKey;

/* ================================================================
 * Batch GTK2/3 → GTK4 widget-API shims (version-independent renames)
 * ================================================================ */

/* gtk_container_set_border_width — removed; use margins. No-op here. */
#ifndef gtk_container_set_border_width
#define gtk_container_set_border_width(w, n)  ((void)0)
#endif

/* GtkIconSize enum values removed in GTK4 (only NORMAL/LARGE remain). */
#ifndef GTK_ICON_SIZE_BUTTON
#define GTK_ICON_SIZE_BUTTON         GTK_ICON_SIZE_NORMAL
#endif
#ifndef GTK_ICON_SIZE_SMALL_TOOLBAR
#define GTK_ICON_SIZE_SMALL_TOOLBAR  GTK_ICON_SIZE_NORMAL
#endif
#ifndef GTK_ICON_SIZE_LARGE_TOOLBAR
#define GTK_ICON_SIZE_LARGE_TOOLBAR  GTK_ICON_SIZE_LARGE
#endif
#ifndef GTK_ICON_SIZE_DND
#define GTK_ICON_SIZE_DND            GTK_ICON_SIZE_LARGE
#endif
#ifndef GTK_ICON_SIZE_DIALOG
#define GTK_ICON_SIZE_DIALOG         GTK_ICON_SIZE_LARGE
#endif
#ifndef GTK_ICON_SIZE_MENU
#define GTK_ICON_SIZE_MENU           GTK_ICON_SIZE_NORMAL
#endif

/* GtkImage icon-name API: GTK4 dropped the trailing size argument. */
#define gtk_image_new_from_icon_name(...)  _compat_image_new_from_icon_name(__VA_ARGS__, NULL)
static inline GtkWidget *_compat_image_new_from_icon_name(const char *name, ...)
    { return (gtk_image_new_from_icon_name)(name); }

#define gtk_image_set_from_icon_name(img, name, ...) \
    (gtk_image_set_from_icon_name)(GTK_IMAGE(img), (name))

/* gtk_image_get_icon_name(img, &name, &size) → single-arg getter. */
#define gtk_image_get_icon_name(img, ...)  _compat_image_get_icon_name(GTK_IMAGE(img), __VA_ARGS__)
static inline void _compat_image_get_icon_name(GtkImage *img, const char **out, ...)
    { if (out) *out = (gtk_image_get_icon_name)(img); }

/* gtk_icon_theme_load_icon — removed in GTK4; no GdkPixbuf path. Stub NULL. */
#ifndef gtk_icon_theme_load_icon
#define gtk_icon_theme_load_icon(theme, name, size, flags, err)  (NULL)
#endif

/* Label line-wrap renamed. */
#define gtk_label_set_line_wrap(l, w)   gtk_label_set_wrap(GTK_LABEL(l), (w))

/* Entry width-chars now on GtkEditable. */
#define gtk_entry_set_width_chars(e, n) \
    gtk_editable_set_width_chars(GTK_EDITABLE(e), (n))

/* GtkFileChooser unselect_all — removed; no-op. */
#ifndef gtk_file_chooser_unselect_all
#define gtk_file_chooser_unselect_all(fc)  ((void)0)
#endif

/* GtkCalendar select_* renamed to set_* and take a GDateTime in 4.x.
 * The legacy integer API is shimmed to no-ops to preserve compilation;
 * callers should migrate to gtk_calendar_select_day(GDateTime*). */
#define gtk_calendar_select_month(cal, mon, yr)  ((void)0)
#define gtk_calendar_select_day(cal, day)        ((void)0)

/* GtkWindow legacy positioning / icon / accel-group APIs removed in GTK4. */
#ifndef gtk_window_set_icon_from_file
#define gtk_window_set_icon_from_file(w, f, err)  (TRUE)
#endif
#ifndef gtk_window_set_position
#define gtk_window_set_position(w, pos)  ((void)0)
#endif
#ifndef GTK_WIN_POS_CENTER_ALWAYS
#define GTK_WIN_POS_NONE             0
#define GTK_WIN_POS_CENTER           1
#define GTK_WIN_POS_MOUSE            2
#define GTK_WIN_POS_CENTER_ALWAYS    3
#define GTK_WIN_POS_CENTER_ON_PARENT 4
#endif
#ifndef gtk_window_add_accel_group
#define gtk_window_add_accel_group(w, g)  ((void)0)
#endif
#ifndef GTK_ACCEL_GROUP
#define GTK_ACCEL_GROUP(g)  (g)
#endif

/* ================================================================
 * GtkRadioButton — removed in GTK4. Radio behaviour is now provided by
 * grouped GtkCheckButtons (gtk_check_button_set_group).
 * ================================================================ */

static inline GtkWidget *_compat_radio_new_with_label(void *group, const char *label)
{
    (void)group;
    return gtk_check_button_new_with_label(label);
}
#define gtk_radio_button_new_with_label(g, l) \
    _compat_radio_new_with_label((g), (l))

static inline GtkWidget *_compat_radio_new_with_label_from_widget(GtkWidget *member, const char *label)
{
    GtkWidget *w = gtk_check_button_new_with_label(label);
    if (member)
        gtk_check_button_set_group(GTK_CHECK_BUTTON(w), GTK_CHECK_BUTTON(member));
    return w;
}
#define gtk_radio_button_new_with_label_from_widget(m, l) \
    _compat_radio_new_with_label_from_widget(GTK_WIDGET(m), (l))

#ifndef GTK_RADIO_BUTTON
#define GTK_RADIO_BUTTON(w)    GTK_CHECK_BUTTON(w)
#endif
#ifndef GTK_IS_RADIO_BUTTON
#define GTK_IS_RADIO_BUTTON(w) GTK_IS_CHECK_BUTTON(w)
#endif

/* Widget classes removed in GTK4 — no instances exist, so type checks
 * always evaluate to FALSE. */
#ifndef GTK_IS_TOOL_BUTTON
#define GTK_IS_TOOL_BUTTON(w)         (FALSE)
#endif
#ifndef GTK_IS_SEPARATOR_MENU_ITEM
#define GTK_IS_SEPARATOR_MENU_ITEM(w) (FALSE)
#endif
#ifndef GTK_IS_MENU_BAR
#define GTK_IS_MENU_BAR(w)            (FALSE)
#endif
#ifndef GTK_IS_MENU
#define GTK_IS_MENU(w)                (FALSE)
#endif

#endif /* GTK4_COMPAT_H */
