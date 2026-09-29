/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_aspectframe.cpp — Cadre conservant un ratio (GtkAspectFrame) (portage FLTK)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_aspectframe.h"
#include <FL/Fl_Group.H>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_aspectframe_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 120, h = 30;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }
    GList *element = NULL;
    gchar *label = NULL;
    if (Attr) label = attributeset_get_first(&element, Attr, ATTR_LABEL);
    Fl_Group *grp = new Fl_Group(0, 0, w, h, nullptr);
    if (label && *label) grp->copy_label(label);
    grp->end();
    return (GtkWidget *)grp;
}

gchar *widget_aspectframe_envvar_construct(GtkWidget *widget) { return g_strdup(""); }
gchar *widget_aspectframe_envvar_all_construct(variable *var) { return g_strdup(""); }
void widget_aspectframe_clear(variable *var) {}
void widget_aspectframe_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *)var->Widget)->redraw();
}
void widget_aspectframe_fileselect(variable *var, const char *n, const char *v) {}
void widget_aspectframe_removeselected(variable *var) {}
void widget_aspectframe_save(variable *var) {}
