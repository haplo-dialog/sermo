/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
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
#include "widget_combobox.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_combobox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_COMBOBOX, NULL, "");
    node->state.list.items       = NULL;
    node->state.list.item_count  = 0;
    /* Parite etalon : combobox et comboboxentry nus ne selectionnent RIEN
     * (le comboboxtext, lui, a sa propre creation qui prend le 1er item). */
    node->state.list.selected_index = -1;

    if (Attr) {
        GList *el = NULL;
        gchar *item = attributeset_get_first(&el, Attr, ATTR_ITEM);
        while (item) {
            if (*item) {
                /* realloc dans une variable à part : en cas d'échec, l'ancien
                 * tableau reste valide au lieu d'être perdu puis déréférencé. */
                char **agrandi = realloc(node->state.list.items,
                    (node->state.list.item_count+1)*sizeof(char*));
                if (!agrandi) break;
                node->state.list.items = agrandi;
                node->state.list.items[node->state.list.item_count++] = strdup(item);
            }
            item = attributeset_get_next(&el, Attr, ATTR_ITEM);
        }
        /* <input> : voir widget_combobox_refresh(). */
    }
    return (GtkWidget *)node;
}

gchar *widget_combobox_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n || !n->state.list.items || n->state.list.item_count == 0) return g_strdup("");
    int idx = n->state.list.selected_index;
    if (idx < 0 || idx >= n->state.list.item_count) return g_strdup("");
    return g_strdup(n->state.list.items[idx]);
}
gchar *widget_combobox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_combobox_envvar_construct(var->Widget);
}
void widget_combobox_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.list.selected_index = 0;
}
void widget_combobox_refresh(variable *var)
{
    if (!var || !var->Attributes) return;
    /* <input> n'est pas implémenté pour ce widget chez l'étalon gtk3sermo :
     * aucune commande exécutée, aucun fichier lu — l'avertissement seul.
     * Jusqu'à la 2.7.0 la directive brute (« Command:… ») partait vers le
     * shell. */
    if (attributeset_is_avail(var->Attributes, ATTR_INPUT))
        g_warning("%s(): <input> not implemented for this widget.", __func__);
}
void widget_combobox_fileselect(variable *var, const char *n, const char *v) {}
void widget_combobox_removeselected(variable *var) {}
void widget_combobox_save(variable *var) {}
