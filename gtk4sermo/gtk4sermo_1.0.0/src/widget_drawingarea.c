/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_drawingarea.c — GtkDrawingArea (GTK4-native)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkDrawingArea : surface de dessin Cairo personnalisée.
 * En GTK4, le rendu se fait via gtk_drawing_area_set_draw_func()
 * (plus de signal "draw").
 *
 * XML :
 *   <drawingarea width="400" height="300">
 *     <variable>MY_CANVAS</variable>
 *     <action signal="draw-command">draw_my_canvas $MY_CANVAS</action>
 *   </drawingarea>
 *
 * Usage avancé : l'action shell "draw-command" reçoit un fd Cairo
 * en tant que variable d'environnement (GTK4_TODO: IPC cairo).
 * Dans cette version initiale, la zone est dessinée en fond uni
 * configurable via l'attribut background-color.
 *
 * Variable exportée : "width:height" de la zone.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#include "config.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "tag_attributes.h"
#include "widget_drawingarea.h"

/* Données partagées entre draw_func et le widget */
typedef struct {
    gdouble r, g, b, a;   /* couleur de fond */
} DrawingAreaData;

static void _draw_func(GtkDrawingArea *area, cairo_t *cr,
                       int width, int height, gpointer user_data)
{
    DrawingAreaData *d = (DrawingAreaData *)user_data;
    if (d) {
        cairo_set_source_rgba(cr, d->r, d->g, d->b, d->a);
        cairo_paint(cr);
    }
}

void widget_drawingarea_clear(variable *var)
{
    gtk_widget_queue_draw(var->Widget);
}

GtkWidget *widget_drawingarea_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget       *widget;
    gint             w = 200, h = 150;
    gchar           *v;
    DrawingAreaData *data;

    data = g_new0(DrawingAreaData, 1);
    data->r = data->g = data->b = 0.9;
    data->a = 1.0;

    if (attr) {
        if ((v = get_tag_attribute(attr, "width")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height"))) h = atoi(v);
        if ((v = get_tag_attribute(attr, "background-color"))) {
            GdkRGBA color;
            if (gdk_rgba_parse(&color, v)) {
                data->r = color.red;
                data->g = color.green;
                data->b = color.blue;
                data->a = color.alpha;
            }
        }
    }

    widget = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(widget), w);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(widget), h);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(widget),
                                   _draw_func, data,
                                   (GDestroyNotify)g_free);
    gtk_widget_set_hexpand(widget, TRUE);
    gtk_widget_set_vexpand(widget, TRUE);

    return widget;
}

gchar *widget_drawingarea_envvar_all_construct(variable *var) { return NULL; }

gchar *widget_drawingarea_envvar_construct(GtkWidget *widget)
{
    return g_strdup_printf("%d:%d",
        gtk_drawing_area_get_content_width(GTK_DRAWING_AREA(widget)),
        gtk_drawing_area_get_content_height(GTK_DRAWING_AREA(widget)));
}

void widget_drawingarea_fileselect(variable *var, const char *name, const char *value)
{
    g_warning("%s(): Not implemented.", __func__);
}

void widget_drawingarea_refresh(variable *var)
{
    gtk_widget_queue_draw(var->Widget);
}

void widget_drawingarea_removeselected(variable *var) {}
void widget_drawingarea_save(variable *var) {}
