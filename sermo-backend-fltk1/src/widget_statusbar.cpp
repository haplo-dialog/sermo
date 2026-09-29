/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_statusbar.cpp — Barre de statut FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <statusbar> → Fl_Output en bas, style barre de statut
 * (FL_FLAT_BOX, fond légèrement enfoncé, texte aligné à gauche)
 * Export : texte affiché
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
#include "widget_statusbar.h"

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_statusbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 400, h = 22;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Output *sb = new Fl_Output(0, 0, w, h, nullptr);
    sb->box(FL_FLAT_BOX);
    sb->color(FL_BACKGROUND_COLOR);
    sb->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    /* Texte initial */
    if (Attr) {
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) sb->value(lbl);
    }

    return (GtkWidget *)sb;
}

gchar *widget_statusbar_envvar_construct(GtkWidget *widget)
{
    Fl_Output *sb = (Fl_Output *)widget;
    if (!sb || !sb->value()) return g_strdup("");
    return g_strdup(sb->value());
}

gchar *widget_statusbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_statusbar_envvar_construct(var->Widget);
}

void widget_statusbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Output *)var->Widget)->value("");
}

void widget_statusbar_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_statusbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_statusbar_removeselected(variable *var) {}
void widget_statusbar_save(variable *var) {}
