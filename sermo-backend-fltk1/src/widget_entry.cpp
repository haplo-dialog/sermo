/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_entry.cpp — Champ de saisie mono-ligne FLTK
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
#include "widget_entry.h"

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_entry_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 200, h = 26;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Input *inp = new Fl_Input(0, 0, w, h, nullptr);

    /* Valeur par défaut */
    if (Attr) {
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) inp->value(def);
    }

    return (GtkWidget *)inp;
}

gchar *widget_entry_envvar_construct(GtkWidget *widget)
{
    Fl_Input *inp = (Fl_Input *)widget;
    if (!inp || !inp->value()) return g_strdup("");
    return g_strdup(inp->value());
}

gchar *widget_entry_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_entry_envvar_construct(var->Widget);
}

void widget_entry_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Input *)var->Widget)->value("");
}

void widget_entry_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    /* Alimenter depuis <input> (Command:/file: décodés par le helper) */
    gchar *text = widget_input_text(var->Attributes);
    if (text) {
        /* la PREMIÈRE ligne, sans CR/LF — étalon gtk3sermo (g_strchomp gardait
         * les lignes suivantes) */
        text[strcspn(text, "\r\n")] = '\0';
        ((Fl_Input *)var->Widget)->value(text);
        g_free(text);
    }
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_entry_fileselect(variable *var, const char *n, const char *v) {}
void widget_entry_removeselected(variable *var) {}
void widget_entry_save(variable *var) {}
