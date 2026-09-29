/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
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
#include "widget_comboboxtext.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Ajoute un élément à la fin de la liste du nœud. */
static void cbt_append(WidgetNode *node, const char *text)
{
    char **items = realloc(node->state.list.items,
        (node->state.list.item_count + 1) * sizeof(char *));
    if (!items) return;
    node->state.list.items = items;
    node->state.list.items[node->state.list.item_count++] = strdup(text);
}

GtkWidget *widget_comboboxtext_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_COMBOBOX, NULL, "");
    node->state.list.items       = NULL;
    node->state.list.item_count  = 0;
    /* Parite etalon : seul le comboboxtext selectionne son 1er item ;
     * le comboboxentry (editable, nu) ne selectionne rien. */
    node->state.list.selected_index = (Type == WIDGET_COMBOBOXTEXT) ? 0 : -1;
    /* <input> et <item> : chargés par widget_comboboxtext_refresh(), que le
     * cœur appelle juste après la création — lire <input> ici aussi
     * exécuterait la commande deux fois. */
    return (GtkWidget *)node;
}

gchar *widget_comboboxtext_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n || !n->state.list.items || n->state.list.item_count == 0) return g_strdup("");
    int idx = n->state.list.selected_index;
    if (idx < 0 || idx >= n->state.list.item_count) return g_strdup("");
    return g_strdup(n->state.list.items[idx]);
}
gchar *widget_comboboxtext_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_comboboxtext_envvar_construct(var->Widget);
}
void widget_comboboxtext_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.list.selected_index = 0;
}
void widget_comboboxtext_refresh(variable *var)
{
    if (!var || !var->Widget || !var->Attributes) return;
    WidgetNode *node = (WidgetNode *)var->Widget;
    GList *el = NULL;
    gchar **lines, *text;
    int i;

    /* Règle de l'étalon gtk3sermo, à chaque refresh : vider, puis un élément
     * par ligne de <input> (commande ou fichier), puis les <item>. */
    for (i = 0; i < node->state.list.item_count; i++)
        free(node->state.list.items[i]);
    free(node->state.list.items);
    node->state.list.items = NULL;
    node->state.list.item_count = 0;

    lines = widget_input_lines(var->Attributes);
    for (i = 0; lines && lines[i]; i++)
        cbt_append(node, lines[i]);
    g_strfreev(lines);
    for (text = attributeset_get_first(&el, var->Attributes, ATTR_ITEM); text;
         text = attributeset_get_next(&el, var->Attributes, ATTR_ITEM))
        if (*text) cbt_append(node, text);

    /* Puis le 1er élément (rien pour un comboboxentry) ; au premier refresh
     * seulement, <default> choisit l'élément de même texte. */
    node->state.list.selected_index = (var->Type == WIDGET_COMBOBOXTEXT) ? 0 : -1;
    if (!node->initialised) {
        el = NULL;
        text = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        for (i = 0; text && i < node->state.list.item_count; i++) {
            if (strcmp(node->state.list.items[i], text) == 0) {
                node->state.list.selected_index = i;
                break;
            }
        }
        node->initialised = TRUE;
    }
}
void widget_comboboxtext_fileselect(variable *var, const char *n, const char *v) {}
void widget_comboboxtext_removeselected(variable *var) {}
void widget_comboboxtext_save(variable *var) {}
