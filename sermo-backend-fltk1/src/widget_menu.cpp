/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menu.cpp — <menu> FLTK : dépile ses <menuitem> et pousse un carrier
 * de menu (label + items) que <menubar> consomme. GPL-2.0-or-later
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
#include "stack.h"
#include "widget_menu.h"
#include "sermo_menu_model.h"
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    SermoMenuCarrier *c = new SermoMenuCarrier();
    c->kind = 1;
    /* <menu label="Fichier"> : le titre est un ATTRIBUT DE BALISE (pas un
     * élément <label>). Repli sur ATTR_LABEL par robustesse. */
    if (attr) {
        const char *tv = get_tag_attribute(attr, "label");
        if (tv && *tv) c->menu.label = tv;
    }
    if (c->menu.label.empty() && Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) c->menu.label = lbl;
    }

    /* Dépiler les <menuitem> (fusionnés en un élément par l'instruction SUM,
     * en ordre document) et récolter leur modèle. */
    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; n++) {
        Fl_Widget *w = (Fl_Widget *)s.widgets[n];
        if (!w) continue;
        SermoMenuCarrier *ic = (SermoMenuCarrier *)w->user_data();
        if (ic && ic->kind == 0) c->menu.items.push_back(ic->item);
    }

    Fl_Box *box = new Fl_Box(0, 0, 0, 0);
    box->user_data(c);
    if (box->parent()) box->parent()->remove(box);
    return (GtkWidget *)box;
}

gchar *widget_menu_envvar_construct(GtkWidget *widget) { return g_strdup(""); }
gchar *widget_menu_envvar_all_construct(variable *var) { return g_strdup(""); }
void widget_menu_clear(variable *var) {}
void widget_menu_refresh(variable *var) {}
void widget_menu_fileselect(variable *var, const char *n, const char *v) {}
void widget_menu_removeselected(variable *var) {}
void widget_menu_save(variable *var) {}
