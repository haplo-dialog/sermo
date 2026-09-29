/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_radiobutton.cpp — Bouton radio FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <radiobutton> → Fl_Round_Button (type RADIO)
 * Export : "true" / "false"
 *
 * Note : le regroupement de boutons radio (mutuelle exclusion) est géré
 * automatiquement par FLTK pour tous les Fl_Round_Button de type RADIO
 * dans le même groupe Fl_Group parent.
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
#include "widget_radiobutton.h"

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_radiobutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
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

    Fl_Round_Button *rb = new Fl_Round_Button(0, 0, w, h, nullptr);
    rb->type(FL_RADIO_BUTTON);
    if (label && *label) rb->copy_label(label);

    /* État initial */
    if (Attr) {
        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && (strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
            rb->value(1);
    }

    return (GtkWidget *)rb;
}

gchar *widget_radiobutton_envvar_construct(GtkWidget *widget)
{
    Fl_Round_Button *rb = (Fl_Round_Button *)widget;
    if (!rb) return g_strdup("false");
    return g_strdup(rb->value() ? "true" : "false");
}

gchar *widget_radiobutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_radiobutton_envvar_construct(var->Widget);
}

void widget_radiobutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Round_Button *)var->Widget)->value(0);
}

void widget_radiobutton_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_radiobutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_radiobutton_removeselected(variable *var) {}
void widget_radiobutton_save(variable *var) {}
