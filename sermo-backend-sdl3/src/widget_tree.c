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
#include "sermo_focus.h"
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
    /* <label>Col1|Col2|…</label> : les en-têtes de colonnes, que le rendu
     * dessine au-dessus des lignes. */
    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
    }
    if (attr) {
        const char *v = get_tag_attribute(attr, "exported-column");
        if (v && atoi(v) > 0) node->colonne_exportee = atoi(v);
    }
    /* <input> et <item> : chargés par widget_tree_refresh(), que le cœur
     * appelle juste après la création — lire <input> ici aussi exécuterait
     * la commande deux fois. */

    node->focus_initial = sermo_focus_initial("tree");
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
    /* La rangée reste entière pour l'affichage ; la variable n'exporte que la
     * colonne « exported-column » (défaut 0 ; colonnes séparées par « | »),
     * comme l'étalon. Jusqu'à la 2.8.1, toujours la 1re. Colonne absente :
     * valeur vide. */
    const char *row = n->state.list.items[idx];
    for (int c = 0; c < n->colonne_exportee; c++) {
        row = strchr(row, '|');
        if (!row) return g_strdup("");
        row++;
    }
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

    if (!n->initialised) {
        /* Ligne choisie à l'ouverture, comme gtk3sermo (sermo_focus.h) :
         * celle de <default> s'il y en a une ; sinon la 1re, seulement si ce
         * widget prend le focus (premier focalisable de la fenêtre) ; sinon
         * aucune. Jusqu'à la 2.8.1, la 1re rangée était toujours choisie. */
        int choix = -1;
        GList *ed = NULL;
        gchar *def = attributeset_get_first(&ed, var->Attributes, ATTR_DEFAULT);
        for (int i = 0; def && *def && i < n->state.list.item_count && choix < 0; i++) {
            const char *it = n->state.list.items[i];
            size_t l = strcspn(it, "|");
            if (strcmp(it, def) == 0 || (strlen(def) == l && strncmp(it, def, l) == 0))
                choix = i;
        }
        if (choix < 0 && n->focus_initial && n->state.list.item_count > 0)
            choix = 0;
        n->state.list.selected_index = choix;
        n->initialised = TRUE;
    }
}
void widget_tree_fileselect(variable *var, const char *name, const char *value) {}
void widget_tree_removeselected(variable *var) {}
void widget_tree_save(variable *var) {}
