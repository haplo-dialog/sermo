/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_drawingarea.c — Zone de dessin EFL (Evas_Object rectangle)
 * sermo — haplo-dialog — GPL-2.0-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_drawingarea.h"
#include <Elementary.h>
#include <Evas.h>

GtkWidget *widget_drawingarea_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas *evas = evas_object_evas_get(win);
    Evas_Object *rect = evas_object_rectangle_add(evas);
    evas_object_color_set(rect, 30, 30, 46, 255);  /* Catppuccin base */
    int ww = 200, hh = 150;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  ww = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) hh = atoi(v);
    }
    evas_object_resize(rect, ww, hh);
    evas_object_show(rect);
    return (GtkWidget *)rect;
}
gchar *widget_drawingarea_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_drawingarea_envvar_all_construct(variable *v) { return NULL; }
void widget_drawingarea_clear(variable *v) {}
void widget_drawingarea_refresh(variable *v) {}
void widget_drawingarea_fileselect(variable *v, const char *n, const char *val) {}
void widget_drawingarea_removeselected(variable *v) {}
void widget_drawingarea_save(variable *v) {}
