/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubutton.c — Bouton qui déroule un menu
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le nœud porte ses <menuitem> et le libellé choisi (state.entry.buf) ; le
 * renderer déroule la liste (render_*.c, WT_MENUBUTTON). Étalon = gtk3.
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
#include "widget_menubutton.h"
#include <stdlib.h>

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

    WidgetNode *node = widget_node_new(WT_MENUBUTTON, NULL, label ? label : "Menu");
    node->state.entry.buf[0] = '\0';     /* aucun choix pour l'instant */

    /* Un seul pop : le cœur coalesce les enfants (instruction SUM). */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *) s.widgets[i]);

    return (GtkWidget *) node;
}

/* Export : le libellé du dernier élément choisi, vide avant tout choix. */
gchar *widget_menubutton_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *) widget;
    if (!n) return g_strdup("");
    return g_strdup(n->state.entry.buf);
}
gchar *widget_menubutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_menubutton_envvar_construct(var->Widget);
}
void widget_menubutton_clear(variable *var)
{
    if (var && var->Widget) ((WidgetNode *) var->Widget)->state.entry.buf[0] = '\0';
}
void widget_menubutton_refresh(variable *var)        { (void) var; }
void widget_menubutton_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_menubutton_removeselected(variable *var) { (void) var; }
void widget_menubutton_save(variable *var)           { (void) var; }
