/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_toolbar.cpp — Barre d'actions (FLTK 1.4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * FLTK n'a pas de barre d'outils : c'est une rangée. Fl_Flex, le même moteur
 * que <hbox> sur ce port, avec un cadre plat pour la démarquer du fond.
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
#include "widget_toolbar.h"
#include <FL/Fl.H>
#include <FL/Fl_Flex.H>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_toolbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    bool vertical = false;
    int  espacement = 4;
    if (attr) {
        const char *v = get_tag_attribute(attr, "orientation");
        if (v && !strcasecmp(v, "vertical")) vertical = true;
        if ((v = get_tag_attribute(attr, "spacing"))) espacement = atoi(v);
    }

    Fl_Flex *barre = new Fl_Flex(0, 0, 800, 34,
                                 vertical ? Fl_Flex::COLUMN : Fl_Flex::ROW);
    barre->end();
    barre->gap(espacement);
    barre->margin(2, 2);
    barre->box(FL_FLAT_BOX);

    stackelement s = pop();
    int haut = 0, larges = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        Fl_Widget *c = (Fl_Widget *) s.widgets[n];
        if (!c) continue;
        barre->add(c);
        if (!c->as_group()) barre->fixed(c, vertical ? c->h() : c->w());
        if (c->h() > haut) haut = c->h();
        larges += c->w() + espacement;
    }
    /* Hauteur naturelle : sans elle la barre garde 34 px arbitraires et se
     * décale du contenu (même piège que <vbox> sur ce port). */
    if (haut > 0) barre->size(vertical ? 120 : 800, vertical ? larges : haut + 4);

    return (GtkWidget *) barre;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_toolbar_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_toolbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_toolbar_envvar_construct(var->Widget);
}
void widget_toolbar_clear(variable *var)   { (void) var; }
void widget_toolbar_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *) var->Widget)->redraw();
}
void widget_toolbar_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_toolbar_removeselected(variable *var) { (void) var; }
void widget_toolbar_save(variable *var)           { (void) var; }
