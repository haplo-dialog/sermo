/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_toolbar.c — Barre d'actions
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le nœud porte ses enfants et l'orientation ; c'est le renderer du port qui
 * dessine la rangée (render_*.c, WT_TOOLBAR). Étalon = gtk3.
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
#include "widget_toolbar.h"
#include <stdlib.h>

GtkWidget *widget_toolbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;
    WidgetNode *node = widget_node_new(WT_TOOLBAR, NULL, "");
    node->state.grid.columns     = 0;     /* 0 = rangée horizontale */
    node->state.grid.col_spacing = 4;

    if (attr) {
        const char *v = get_tag_attribute(attr, "orientation");
        if (v && !strcasecmp(v, "vertical")) node->state.grid.columns = 1;
        if ((v = get_tag_attribute(attr, "spacing"))) node->state.grid.col_spacing = atoi(v);
    }

    /* Un seul pop : le cœur coalesce les enfants (instruction SUM). */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *) s.widgets[i]);

    return (GtkWidget *) node;
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
void widget_toolbar_clear(variable *var)          { (void) var; }
void widget_toolbar_refresh(variable *var)        { (void) var; }
void widget_toolbar_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_toolbar_removeselected(variable *var) { (void) var; }
void widget_toolbar_save(variable *var)           { (void) var; }
