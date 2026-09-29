/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_drawingarea.cpp — Zone de dessin FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <drawingarea> → Fl_Box (zone vide dessinable aux dimensions demandées)
 * Attributs : width-request, height-request.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_drawingarea.h"

#include <stdlib.h>

GtkWidget *widget_drawingarea_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 200, h = 150;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Box *da = new Fl_Box(0, 0, w, h, nullptr);
    da->box(FL_FLAT_BOX);
    da->color(FL_BACKGROUND2_COLOR);

    return (GtkWidget *)da;
}

gchar *widget_drawingarea_envvar_construct(GtkWidget *widget)  { return g_strdup(""); }
gchar *widget_drawingarea_envvar_all_construct(variable *var)  { return g_strdup(""); }
void   widget_drawingarea_clear(variable *var)                 {}
void   widget_drawingarea_refresh(variable *var)               { if (var && var->Widget) ((Fl_Widget *)var->Widget)->redraw(); }
void   widget_drawingarea_fileselect(variable *var, const char *n, const char *v) {}
void   widget_drawingarea_removeselected(variable *var)        {}
void   widget_drawingarea_save(variable *var)                  {}
