/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_grid.c — Conteneur de mise en page en tableau
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <grid columns="N"> range ses enfants EN FLOT : ordre du document, retour à
 * la ligne tous les N. Le nœud ne fait que porter le contenu et le nombre de
 * colonnes ; c'est le renderer du port qui dispose (render_*.c, WT_GRID).
 * Étalon = gtk3 (GtkGrid).
 *
 * ⚠️ <grid> n'est pas <table> : <table> est la liste à colonnes des données.
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
#include "widget_grid.h"
#include <stdlib.h>

static int grid_attr_int(tag_attr *attr, const char *nom, int repli)
{
    if (!attr) return repli;
    const char *v = get_tag_attribute(attr, nom);
    if (!v || !*v) return repli;
    int n = atoi(v);
    return (n >= 0) ? n : repli;
}

GtkWidget *widget_grid_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;
    WidgetNode *node = widget_node_new(WT_GRID, NULL, "");

    int columns = grid_attr_int(attr, "columns", 0);
    if (columns <= 0) {
        fprintf(stderr, "sermo: <grid> sans attribut columns= utilisable : "
                        "une seule colonne.\n");
        columns = 1;
    }
    node->state.grid.columns     = columns;
    node->state.grid.row_spacing = grid_attr_int(attr, "row-spacing", 4);
    node->state.grid.col_spacing = grid_attr_int(attr, "column-spacing", 8);
    node->state.grid.homogeneous = FALSE;
    if (attr) {
        const char *h = get_tag_attribute(attr, "homogeneous");
        if (h && (!strcasecmp(h, "true") || !strcasecmp(h, "yes") || atoi(h) == 1))
            node->state.grid.homogeneous = TRUE;
    }

    /* Un seul pop : le cœur coalesce les enfants (instruction SUM). */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *) s.widgets[i]);

    return (GtkWidget *) node;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_grid_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_grid_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_grid_envvar_construct(var->Widget);
}
void widget_grid_clear(variable *var)          { (void) var; }
void widget_grid_refresh(variable *var)        { (void) var; }
void widget_grid_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_grid_removeselected(variable *var) { (void) var; }
void widget_grid_save(variable *var)           { (void) var; }
