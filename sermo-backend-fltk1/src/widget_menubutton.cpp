/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubutton.cpp — Bouton qui déroule un menu (FLTK 1.4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Fl_Menu_Button est exactement ce tag. <menuitem> pose son modèle en
 * user_data() d'un Fl_Box invisible (sermo_menu_model.h) : on le relit.
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
#include "widget_menubutton.h"
#include "sermo_menu_model.h"
#include <FL/Fl.H>
#include <FL/Fl_Menu_Button.H>
#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>

/* execute_action vient de actions.h : une seule déclaration, sinon violation
 * de l'ODR (type de retour et constance divergents). */
#include "actions.h"

namespace {
/* Ce que retient le bouton : les commandes, et le dernier choix. */
struct SermoMenuChoix {
    std::vector<std::string> cmds;
    std::string              choisi;
};
std::vector<SermoMenuChoix *> g_choix;   /* possédés jusqu'à la fin du programme */

void entree_choisie(Fl_Widget *w, void *d)
{
    Fl_Menu_Button  *mb = (Fl_Menu_Button *) w;
    SermoMenuChoix  *c  = (SermoMenuChoix *) d;
    const Fl_Menu_Item *mi = mb->mvalue();
    if (!c || !mi || !mi->label()) return;
    c->choisi = mi->label();
    int i = mb->value();
    if (i >= 0 && i < (int) c->cmds.size() && !c->cmds[i].empty())
        execute_action((GtkWidget *) mb, (char *) c->cmds[i].c_str(), NULL);
}
} /* namespace */

GtkWidget *widget_menubutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;

    const char *label = NULL;
    if (Attr) {
        GList *el = NULL;
        gchar *l = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (l && *l) label = l;
    }
    if (!label && attr) { const char *v = get_tag_attribute(attr, "label"); if (v) label = v; }

    Fl_Menu_Button *mb = new Fl_Menu_Button(0, 0, 140, 30);
    mb->copy_label(label ? label : "Menu");

    SermoMenuChoix *c = new SermoMenuChoix();
    g_choix.push_back(c);

    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n) {
        Fl_Widget *porteur = (Fl_Widget *) s.widgets[n];
        if (!porteur) continue;
        SermoMenuCarrier *mc = (SermoMenuCarrier *) porteur->user_data();
        if (!mc) continue;
        if (mc->item.separator) continue;
        mb->add(mc->item.label.c_str(), 0, entree_choisie, c);
        c->cmds.push_back(mc->item.cmd);
    }
    mb->user_data(c);   /* l'export lit le choix ici */
    return (GtkWidget *) mb;
}

/* Export : le libellé du dernier élément choisi, vide avant tout choix. */
gchar *widget_menubutton_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("");
    SermoMenuChoix *c = (SermoMenuChoix *) ((Fl_Widget *) widget)->user_data();
    return g_strdup(c ? c->choisi.c_str() : "");
}
gchar *widget_menubutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_menubutton_envvar_construct(var->Widget);
}
void widget_menubutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    SermoMenuChoix *c = (SermoMenuChoix *) ((Fl_Widget *) var->Widget)->user_data();
    if (c) c->choisi.clear();
}
void widget_menubutton_refresh(variable *var)        { (void) var; }
void widget_menubutton_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_menubutton_removeselected(variable *var) { (void) var; }
void widget_menubutton_save(variable *var)           { (void) var; }
