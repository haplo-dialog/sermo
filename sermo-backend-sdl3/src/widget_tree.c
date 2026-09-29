/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_tree.c — Widget tree SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * SDL3_TODO: arbre complet non implémenté dans ce stub.
 * Les items sont chargés comme une liste plate (même state.list).
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
#include "widget_tree.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Ajoute une rangée à la fin de la liste plate du nœud. */
static void tree_append(WidgetNode *node, const char *text)
{
    char **items = realloc(node->state.list.items,
        (node->state.list.item_count + 1) * sizeof(char *));
    if (!items) return;
    node->state.list.items = items;
    node->state.list.items[node->state.list.item_count++] = strdup(text);
}

GtkWidget *widget_tree_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_LIST, NULL, "");
    node->state.list.items        = NULL;
    node->state.list.item_count   = 0;
    node->state.list.selected_index = -1;
    /* <input> et <item> : chargés par widget_tree_refresh(), que le cœur
     * appelle juste après la création — lire <input> ici aussi exécuterait
     * la commande deux fois. */

    return (GtkWidget *)node;
}

gchar *widget_tree_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n || !n->state.list.items || n->state.list.item_count == 0)
        return g_strdup("");
    int idx = n->state.list.selected_index;
    if (idx < 0 || idx >= n->state.list.item_count)
        return g_strdup("");
    /* La rangée reste entière pour l'affichage en liste plate ; la variable
     * n'exporte que sa 1re colonne (colonnes séparées par « | »), comme
     * l'étalon dont exported-column vaut 0 par défaut. */
    const char *row = n->state.list.items[idx];
    return g_strndup(row, strcspn(row, "|"));
}

gchar *widget_tree_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_tree_envvar_construct(var->Widget);
}

void widget_tree_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    for (int i = 0; i < n->state.list.item_count; i++)
        free(n->state.list.items[i]);
    free(n->state.list.items);
    n->state.list.items = NULL;
    n->state.list.item_count = 0;
    n->state.list.selected_index = -1;
}

void widget_tree_refresh(variable *var)
{
    if (!var || !var->Widget || !var->Attributes) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    GList *el = NULL;
    gchar **lines, *item;

    /* Règle de l'étalon gtk3sermo : à chaque refresh l'arbre est vidé, puis
     * une rangée par ligne de <input> (commande ou fichier), puis une par
     * <item>. */
    widget_tree_clear(var);
    lines = widget_input_lines(var->Attributes);
    for (int i = 0; lines && lines[i]; i++)
        tree_append(n, lines[i]);
    g_strfreev(lines);
    for (item = attributeset_get_first(&el, var->Attributes, ATTR_ITEM); item;
         item = attributeset_get_next(&el, var->Attributes, ATTR_ITEM))
        if (*item) tree_append(n, item);

    /* Au premier refresh seulement, la 1re rangée est choisie, qu'elle vienne
     * de <item> ou de <input>. Chez l'étalon, elle ressort grâce au focus
     * clavier pris à l'ouverture (ce port ne modélise pas le focus) ; un
     * refresh ultérieur vide l'arbre et la sélection avec (mesuré). */
    if (!n->initialised) {
        if (n->state.list.item_count > 0)
            n->state.list.selected_index = 0;
        n->initialised = TRUE;
    }
}
void widget_tree_fileselect(variable *var, const char *name, const char *value) {}
void widget_tree_removeselected(variable *var) {}
void widget_tree_save(variable *var) {}
