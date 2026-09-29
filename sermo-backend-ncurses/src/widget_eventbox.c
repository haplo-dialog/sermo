/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_eventbox.c — Conteneur capteur d'évènements ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <eventbox> est un CONTENEUR : il emballe un enfant unique pour lui donner
 * une zone cliquable. Le stub précédent ne dépilait pas la pile — l'enfant
 * était PERDU (mesuré : le <text> d'un eventbox ne s'affichait plus du tout).
 * Ici le nœud dépile ses enfants comme une vbox, et porte l'<action>
 * éventuelle : en terminal, la zone devient activable au clavier.
 *
 * Export : chaîne VIDE, comme l'étalon gtk3 (widget_eventbox_envvar_construct).
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "ncurses-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_eventbox.h"
#include <stdlib.h>

GtkWidget *widget_eventbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) attr; (void) Type;
    WidgetNode *node = widget_node_new(WT_EVENTBOX, NULL, "");

    /* Même discipline de pile que vbox : le cœur fusionne les enfants en UN
     * élément (instruction SUM), donc UN SEUL pop, dans l'ordre du document. */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *) s.widgets[i]);

    if (Attr) {
        GList *el = NULL;
        gchar *act = attributeset_get_first(&el, Attr, ATTR_ACTION);
        if (act && *act) node->action = strdup(act);
    }
    return (GtkWidget *) node;
}

gchar *widget_eventbox_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");            /* étalon gtk3 : valeur vide */
}
gchar *widget_eventbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_eventbox_envvar_construct(var->Widget);
}
void widget_eventbox_clear(variable *var)          { (void) var; }
void widget_eventbox_refresh(variable *var)        { (void) var; }
void widget_eventbox_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_eventbox_removeselected(variable *var) { (void) var; }
void widget_eventbox_save(variable *var)           { (void) var; }
