/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_infobar.cpp — Barre d'information contextuelle (GtkInfoBar) (portage FLTK)
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
#include "widget_infobar.h"
#include <FL/Fl_Box.H>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_infobar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
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
    Fl_Box *wdg = new Fl_Box(0, 0, w, h);
    if (label && *label) wdg->copy_label(label);
    return (GtkWidget *)wdg;
}

gchar *widget_infobar_envvar_construct(GtkWidget *widget)
{
    /* Parité étalon : l'infobar exporte son texte (IB="attention") */
    if (!widget) return g_strdup("");
    const char *l = ((Fl_Widget *)widget)->label();
    return g_strdup(l ? l : "");
}
gchar *widget_infobar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return g_strdup("");
    return widget_infobar_envvar_construct((GtkWidget *)var->Widget);
}
void widget_infobar_clear(variable *var) {}
void widget_infobar_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *)var->Widget)->redraw();
}
void widget_infobar_fileselect(variable *var, const char *n, const char *v) {}
void widget_infobar_removeselected(variable *var) {}
void widget_infobar_save(variable *var) {}
