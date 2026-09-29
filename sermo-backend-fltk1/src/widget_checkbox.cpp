/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_checkbox.cpp — Case à cocher FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <checkbox> → Fl_Check_Button
 * Export : "true" si coché, "false" sinon.
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
#include "widget_checkbox.h"

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 150, h = 26;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    gchar *label = NULL;
    if (Attr)
        label = attributeset_get_first(&element, Attr, ATTR_LABEL);

    Fl_Check_Button *cb = new Fl_Check_Button(0, 0, w, h, nullptr);
    if (label && *label) cb->copy_label(label);

    /* État initial */
    if (Attr) {
        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && (strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
            cb->value(1);
    }

    return (GtkWidget *)cb;
}

gchar *widget_checkbox_envvar_construct(GtkWidget *widget)
{
    Fl_Check_Button *cb = (Fl_Check_Button *)widget;
    if (!cb) return g_strdup("false");
    return g_strdup(cb->value() ? "true" : "false");
}

gchar *widget_checkbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_checkbox_envvar_construct(var->Widget);
}

void widget_checkbox_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Check_Button *)var->Widget)->value(0);
}

void widget_checkbox_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_checkbox_fileselect(variable *var, const char *n, const char *v) {}
void widget_checkbox_removeselected(variable *var) {}
void widget_checkbox_save(variable *var) {}
