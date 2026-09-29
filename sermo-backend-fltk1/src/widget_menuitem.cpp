/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_menuitem.cpp — Item de menu FLTK : carrier de modèle poussé sur la
 * pile, consommé par <menu>/<menubar>. GPL-2.0-or-later */
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
#include "widget_menuitem.h"
#include "sermo_menu_model.h"
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <string.h>
#include <stdlib.h>

/* Un <menuitem> ne dessine rien lui-même : il pousse un Fl_Box invisible dont
 * le user_data() porte le modèle (label/icône/action/coche). <menu> le dépile. */
GtkWidget *widget_menuitem_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    SermoMenuCarrier *c = new SermoMenuCarrier();
    c->kind = 0;
    if (Type == WIDGET_MENUITEMSEPARATOR) c->item.separator = true;

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) c->item.label = lbl;
        el = NULL;
        /* repli : <menuitem label="..."> en attribut de balise */
        if (c->item.label.empty() && attr) {
            const char *tv = get_tag_attribute(attr, "label");
            if (tv && *tv) c->item.label = tv;
        }
        gchar *cmd = attributeset_get_first(&el, Attr, ATTR_ACTION);
        if (cmd && *cmd) c->item.cmd = cmd;
    }
    if (attr) {
        const char *icon = get_tag_attribute(attr, "icon");
        if (!icon) icon = get_tag_attribute(attr, "icon-name");
        if (!icon) icon = get_tag_attribute(attr, "image-icon");
        if (!icon) icon = get_tag_attribute(attr, "stock");
        if (!icon) icon = get_tag_attribute(attr, "stock-id");
        if (icon && *icon) c->item.icon = icon;
        const char *v = get_tag_attribute(attr, "checkbox");
        if (!v) v = get_tag_attribute(attr, "radiobutton");
        if (v) { c->item.has_check = true;
                 c->item.check_val = (strcasecmp(v, "true") == 0 || strcmp(v, "1") == 0); }
    }

    Fl_Box *box = new Fl_Box(0, 0, 0, 0);
    box->user_data(c);
    /* ne pas laisser le carrier dans le groupe courant (il fausserait la mise
     * en page ; il n'est qu'un porteur de données pour <menu>) */
    if (box->parent()) box->parent()->remove(box);
    return (GtkWidget *)box;
}

gchar *widget_menuitem_envvar_construct(GtkWidget *w)
{
    if (!w) return NULL;
    SermoMenuCarrier *c = (SermoMenuCarrier *)((Fl_Widget *)w)->user_data();
    if (!c || c->kind != 0 || !c->item.has_check) return NULL;
    return g_strdup(c->item.check_val ? "true" : "false");
}
gchar *widget_menuitem_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_menuitem_envvar_construct(v->Widget) : NULL; }
void widget_menuitem_clear(variable *v) {}
void widget_menuitem_refresh(variable *v) {}
void widget_menuitem_fileselect(variable *v, const char*, const char*) {}
void widget_menuitem_removeselected(variable *v) {}
void widget_menuitem_save(variable *v) {}
