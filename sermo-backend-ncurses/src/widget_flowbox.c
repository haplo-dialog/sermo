/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_flowbox.c — Rangement en lignes
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le nœud porte ses enfants et le nombre de colonnes ; le renderer range en
 * lignes (render_*.c, WT_FLOWBOX). ⚠️ Comme sur qt6 et fltk1, le nombre de
 * colonnes est FIXÉ par max-children-per-line : pas de reflux dynamique. */
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
#include "widget_flowbox.h"
#include <stdlib.h>

GtkWidget *widget_flowbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;
    WidgetNode *node = widget_node_new(WT_FLOWBOX, NULL, "");

    node->state.grid.columns     = 4;
    node->state.grid.col_spacing = 6;
    node->state.grid.row_spacing = 6;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "max-children-per-line"))) node->state.grid.columns = atoi(v);
        if ((v = get_tag_attribute(attr, "column-spacing")))        node->state.grid.col_spacing = atoi(v);
        if ((v = get_tag_attribute(attr, "row-spacing")))           node->state.grid.row_spacing = atoi(v);
    }
    if (node->state.grid.columns < 1) node->state.grid.columns = 1;

    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *) s.widgets[i]);
    return (GtkWidget *) node;
}

/* Export : l'index de l'enfant sélectionné — ce port n'en sélectionne aucun. */
gchar *widget_flowbox_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_flowbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_flowbox_envvar_construct(var->Widget);
}
void widget_flowbox_clear(variable *var)          { (void) var; }
void widget_flowbox_refresh(variable *var)        { (void) var; }
void widget_flowbox_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_flowbox_removeselected(variable *var) { (void) var; }
void widget_flowbox_save(variable *var)           { (void) var; }
