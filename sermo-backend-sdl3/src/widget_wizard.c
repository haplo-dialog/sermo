/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_wizard.c — Suite d'étapes
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le nœud porte ses étapes et l'index courant ; le renderer dessine l'étape
 * courante plus les trois boutons (render_*.c, WT_WIZARD). Étalon = gtk3.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_wizard.h"
#include <stdlib.h>

GtkWidget *widget_wizard_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) attr; (void) Type;
    WidgetNode *node = widget_node_new(WT_WIZARD, NULL, "");

    node->state.notebook.current_tab = 0;
    /* Les <action> du wizard : « Terminer » les jouera. */
    if (Attr && attributeset_is_avail(Attr, ATTR_ACTION))
        node->actions_attr = Attr;

    /* Un seul pop : le cœur coalesce les enfants (instruction SUM). */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *) s.widgets[i]);

    return (GtkWidget *) node;
}

/* Export : l'index de l'étape courante, comme <stack> (étalon gtk3). */
gchar *widget_wizard_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *) widget;
    char buf[16];
    if (!n) return g_strdup("0");
    snprintf(buf, sizeof(buf), "%d", n->state.notebook.current_tab);
    return g_strdup(buf);
}
gchar *widget_wizard_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_wizard_envvar_construct(var->Widget);
}
void widget_wizard_clear(variable *var)          { (void) var; }
void widget_wizard_refresh(variable *var)        { (void) var; }
void widget_wizard_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_wizard_removeselected(variable *var) { (void) var; }
void widget_wizard_save(variable *var)           { (void) var; }
