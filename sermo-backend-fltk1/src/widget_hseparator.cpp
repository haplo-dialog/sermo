/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hseparator.cpp — Séparateur horizontal FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <hseparator> → Fl_Box avec FL_ENGRAVED_BOX (ligne gravée horizontale)
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
#include "widget_hseparator.h"

#include <stdlib.h>

GtkWidget *widget_hseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 300, h = 4;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Box *sep = new Fl_Box(0, 0, w, h, nullptr);
    sep->box(FL_ENGRAVED_BOX);

    return (GtkWidget *)sep;
}

gchar *widget_hseparator_envvar_construct(GtkWidget *widget)  { return g_strdup(""); }
gchar *widget_hseparator_envvar_all_construct(variable *var)  { return g_strdup(""); }
void   widget_hseparator_clear(variable *var)                 {}
void   widget_hseparator_refresh(variable *var)               { if (var && var->Widget) ((Fl_Widget *)var->Widget)->redraw(); }
void   widget_hseparator_fileselect(variable *var, const char *n, const char *v) {}
void   widget_hseparator_removeselected(variable *var)        {}
void   widget_hseparator_save(variable *var)                  {}
