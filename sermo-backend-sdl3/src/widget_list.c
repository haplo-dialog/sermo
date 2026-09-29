/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_list.c — Widget list SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_list.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Ajoute une rangée à la fin de la liste du nœud. */
static void list_append(WidgetNode *node, const char *text)
{
    char **items = realloc(node->state.list.items,
        (node->state.list.item_count + 1) * sizeof(char *));
    if (!items) return;
    node->state.list.items = items;
    node->state.list.items[node->state.list.item_count++] = strdup(text);
}

GtkWidget *widget_list_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_LIST, NULL, "");
    node->state.list.items        = NULL;
    node->state.list.item_count   = 0;
    node->state.list.selected_index = -1;
    /* <input> et <item> : chargés par widget_list_refresh(), que le cœur
     * appelle juste après la création — lire <input> ici aussi exécuterait
     * la commande deux fois. */

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "variable")) && *v)
            node->var_name = strdup(v);
    }

    return (GtkWidget *)node;
}

gchar *widget_list_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n || !n->state.list.items || n->state.list.item_count == 0)
        return g_strdup("");
    int idx = n->state.list.selected_index;
    if (idx < 0 || idx >= n->state.list.item_count)
        return g_strdup("");
    return g_strdup(n->state.list.items[idx]);
}

gchar *widget_list_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_list_envvar_construct(var->Widget);
}

void widget_list_clear(variable *var)
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

void widget_list_refresh(variable *var)
{
    if (!var || !var->Widget || !var->Attributes) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    GList *el = NULL;
    gchar **lines, *item;

    /* Règle de l'étalon gtk3sermo : chaque refresh AJOUTE, sans vider, une
     * rangée par ligne de <input> (commande ou fichier), puis une par <item>. */
    lines = widget_input_lines(var->Attributes);
    for (int i = 0; lines && lines[i]; i++)
        list_append(n, lines[i]);
    g_strfreev(lines);
    for (item = attributeset_get_first(&el, var->Attributes, ATTR_ITEM); item;
         item = attributeset_get_next(&el, var->Attributes, ATTR_ITEM))
        if (*item) list_append(n, item);

    /* Au premier refresh seulement, la 1re rangée est choisie, qu'elle vienne
     * de <item> ou de <input>. Chez l'étalon, elle ressort grâce au focus
     * clavier pris à l'ouverture (ce port ne modélise pas le focus) ; un
     * refresh ultérieur garde la sélection, et après un clear il n'y en a
     * plus (mesuré). */
    if (!n->initialised) {
        if (n->state.list.item_count > 0)
            n->state.list.selected_index = 0;
        n->initialised = TRUE;
    }
}
void widget_list_fileselect(variable *var, const char *name, const char *value) {}
void widget_list_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    int idx = n->state.list.selected_index;
    if (idx < 0 || idx >= n->state.list.item_count) return;
    free(n->state.list.items[idx]);
    for (int i = idx; i < n->state.list.item_count - 1; i++)
        n->state.list.items[i] = n->state.list.items[i + 1];
    n->state.list.item_count--;
    if (n->state.list.selected_index >= n->state.list.item_count)
        n->state.list.selected_index = n->state.list.item_count - 1;
}
void widget_list_save(variable *var) {}
