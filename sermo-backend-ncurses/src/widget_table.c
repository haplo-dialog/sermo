/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_table.c — Widget table ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * SDL3_TODO: ImGui::BeginTable disponible — câblage dans render.cpp.
 * Ce stub crée un container avec les enfants popés.
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
#include "widget_table.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Ajoute une rangée : une cellule par champ séparé par « | », champs vides
 * compris, comme l'étalon ; au-delà des colonnes du <label> les champs sont
 * ignorés, ceux qui manquent restent vides. Pas de g_strsplit : celui de la
 * couche de compatibilité passe par strtok et fusionne les champs vides ; et
 * l'ancienne boucle lisait après le NULL final de son tableau dès qu'une
 * ligne avait moins de champs que la table de colonnes. */
static void table_append_row(WidgetNode *node, const char *line)
{
    int cols = node->state.table.cols, r = node->state.table.rows, c;
    const char *p = line;
    char **cells;

    if (cols < 1) return;
    cells = realloc(node->state.table.cells,
        (size_t)(r + 1) * (size_t)cols * sizeof(char *));
    if (!cells) return;
    node->state.table.cells = cells;
    for (c = 0; c < cols; c++) {
        const char *bar = p ? strchr(p, '|') : NULL;
        cells[r * cols + c] = !p ? strdup("")
                            : bar ? strndup(p, (size_t)(bar - p)) : strdup(p);
        p = bar ? bar + 1 : NULL;
    }
    node->state.table.rows = r + 1;
}

GtkWidget *widget_table_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_TABLE, NULL, "table");

    /* Une table n'a PAS d'enfants empiles : <label> = en-tetes, chaque
     * <item> = une ligne (l'ancien pop() sous-vidait la pile -> abort
     * sur TOUT dialogue a table). */
    {
        GList *el = NULL;
        gchar *hdr = Attr ? attributeset_get_first(&el, Attr, ATTR_LABEL) : NULL;
        int cols = 1;
        if (hdr && *hdr) {
            const char *c2;
            for (c2 = hdr; *c2; c2++) if (*c2 == '|') cols++;
        }
        node->state.table.cols = cols;
        node->state.table.rows = 0;
        node->state.table.headers = NULL;
        node->state.table.cells = NULL;
        if (hdr && *hdr) {
            gchar **hs = g_strsplit(hdr, "|", -1);
            int n2; for (n2 = 0; hs[n2]; n2++);
            node->state.table.headers = hs;      /* garde le g_strsplit */
            node->state.table.cols = n2;
        }
        /* Les rangées (<input> puis <item>) : chargées par
         * widget_table_refresh(), que le cœur appelle juste après la
         * création — lire <input> ici aussi exécuterait la commande deux
         * fois. */
    }

    return (GtkWidget *)node;
}

gchar *widget_table_envvar_construct(GtkWidget *widget)
{
    /* Parite etalon : 1re ligne selectionnee par defaut, la variable
     * exporte sa PREMIERE colonne. */
    WidgetNode *n = (WidgetNode *)widget;
    if (!n || n->state.table.rows < 1 || n->state.table.cols < 1 ||
        !n->state.table.cells || !n->state.table.cells[0])
        return g_strdup("");
    return g_strdup(n->state.table.cells[0]);
}

gchar *widget_table_envvar_all_construct(variable *var)
{
    return g_strdup("");
}

void widget_table_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    /* Vide les rangées, en-têtes gardés. Refresh AJOUTE des rangées : sans
     * ce vidage, « clear puis refresh » ferait grossir la table au lieu de
     * la recharger comme chez l'étalon. */
    if (n->state.table.cells)
        for (int i = 0; i < n->state.table.rows * n->state.table.cols; i++)
            free(n->state.table.cells[i]);
    free(n->state.table.cells);
    n->state.table.cells = NULL;
    n->state.table.rows = 0;
}

void widget_table_refresh(variable *var)
{
    if (!var || !var->Widget || !var->Attributes) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    GList *el = NULL;
    gchar **lines, *item;

    /* Règle de l'étalon gtk3sermo : chaque refresh AJOUTE, sans vider, une
     * rangée par ligne de <input> (commande ou fichier), puis une par <item>.
     * La valeur exportée reste la 1re cellule de la 1re rangée. */
    lines = widget_input_lines(var->Attributes);
    for (int i = 0; lines && lines[i]; i++)
        table_append_row(n, lines[i]);
    g_strfreev(lines);
    for (item = attributeset_get_first(&el, var->Attributes, ATTR_ITEM); item;
         item = attributeset_get_next(&el, var->Attributes, ATTR_ITEM))
        if (*item) table_append_row(n, item);
}
void widget_table_fileselect(variable *var, const char *name, const char *value) {}
void widget_table_removeselected(variable *var) {}
void widget_table_save(variable *var) {}
