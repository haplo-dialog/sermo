/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_togglebutton.cpp — Bouton bascule FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <togglebutton> → Fl_Toggle_Button
 * Export : "true" / "false"
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
#include "widget_togglebutton.h"

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_togglebutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 120, h = 30;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    gchar *label = NULL;
    if (Attr)
        label = attributeset_get_first(&element, Attr, ATTR_LABEL);

    Fl_Toggle_Button *btn = new Fl_Toggle_Button(0, 0, w, h, nullptr);
    if (label && *label) btn->copy_label(label);

    /* État initial */
    if (Attr) {
        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && (strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
            btn->value(1);
    }

    return (GtkWidget *)btn;
}

gchar *widget_togglebutton_envvar_construct(GtkWidget *widget)
{
    Fl_Toggle_Button *btn = (Fl_Toggle_Button *)widget;
    if (!btn) return g_strdup("false");
    return g_strdup(btn->value() ? "true" : "false");
}

gchar *widget_togglebutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_togglebutton_envvar_construct(var->Widget);
}

void widget_togglebutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Toggle_Button *)var->Widget)->value(0);
}

void widget_togglebutton_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_togglebutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_togglebutton_removeselected(variable *var) {}
void widget_togglebutton_save(variable *var) {}
