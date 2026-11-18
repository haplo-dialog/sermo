/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "html-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_comboboxtext.h"
#include "safe_exec.h"
#include "jouer_actions.h"
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

/* Le texte du widget : l'élément choisi, sinon le texte libre d'un
 * <comboboxentry> (son <default> absent de la liste), rangé dans texte_libre
 * (entier) et dans state.list.filter (255 octets, pour l'affichage). */
static const char *cbt_texte(WidgetNode *n)
{
    int idx = n->state.list.selected_index;
    if (n->state.list.items && idx >= 0 && idx < n->state.list.item_count)
        return n->state.list.items[idx];
    return n->texte_libre ? n->texte_libre : n->state.list.filter;
}

static void cbt_vider_liste(WidgetNode *node)
{
    int i;
    for (i = 0; i < node->state.list.item_count; i++)
        free(node->state.list.items[i]);
    free(node->state.list.items);
    node->state.list.items = NULL;
    node->state.list.item_count = 0;
}

/* Le champ de saisie de l'étalon, vidé par clear:, refresh: et removeselected:. */
static void cbt_vider_texte_libre(WidgetNode *node)
{
    free(node->texte_libre);
    node->texte_libre = NULL;
    node->state.list.filter[0] = '\0';
}

/* Règle de l'étalon gtk3sermo : les <action> de la liste jouent (signal
 * « changed ») quand son texte n'est plus le même après qu'avant. Jusqu'à la
 * 3.3.0 ce port ne les jouait jamais. avant : copie à libérer ici. */
static void cbt_jouer_si_change(variable *var, WidgetNode *node, char *avant)
{
    int change = avant && strcmp(avant, cbt_texte(node)) != 0;
    free(avant);
    if (change)
        jouer_actions((GtkWidget *)node, var->Attributes, "changed", NULL);
}

GtkWidget *widget_comboboxtext_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_COMBOBOX, NULL, "");
    widget_node_bind_variable(node, Attr, attr);   /* nom du champ du formulaire (mode serve) */
    node->state.list.items       = NULL;
    node->state.list.item_count  = 0;
    node->actions_attr = (void *)Attr;   /* ses <action> : jouées quand le choix change */
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
    if (!n) return g_strdup("");
    return g_strdup(cbt_texte(n));
}
gchar *widget_comboboxtext_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_comboboxtext_envvar_construct(var->Widget);
}
void widget_comboboxtext_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *node = (WidgetNode *)var->Widget;
    char *avant = strdup(cbt_texte(node));

    /* Comme l'étalon : la liste est vidée (et le champ d'un <comboboxentry>),
     * plus rien n'est choisi. Jusqu'à la 3.3.0 ce port gardait les éléments et
     * choisissait le premier. */
    cbt_vider_liste(node);
    node->state.list.selected_index = -1;
    if (var->Type == WIDGET_COMBOBOXENTRY) cbt_vider_texte_libre(node);
    cbt_jouer_si_change(var, node, avant);
}
void widget_comboboxtext_refresh(variable *var)
{
    if (!var || !var->Widget || !var->Attributes) return;
    WidgetNode *node = (WidgetNode *)var->Widget;
    GList *el = NULL;
    gchar **lines, *text;
    char *avant = NULL;
    int i;

    /* Au premier refresh (la création), l'étalon n'a pas encore branché ses
     * signaux : aucune action ne joue, même si <default> choisit un élément. */
    if (node->initialised) avant = strdup(cbt_texte(node));

    /* Règle de l'étalon gtk3sermo, à chaque refresh : vider, puis un élément
     * par ligne de <input> (commande ou fichier), puis les <item>. */
    cbt_vider_liste(node);
    if (node->initialised && var->Type == WIDGET_COMBOBOXENTRY)
        cbt_vider_texte_libre(node);

    lines = widget_input_lines(var->Attributes);
    for (i = 0; lines && lines[i]; i++)
        cbt_append(node, lines[i]);
    g_strfreev(lines);
    for (text = attributeset_get_first(&el, var->Attributes, ATTR_ITEM); text;
         text = attributeset_get_next(&el, var->Attributes, ATTR_ITEM))
        if (*text) cbt_append(node, text);

    /* Puis le 1er élément (rien pour un comboboxentry) ; au premier refresh
     * seulement, <default> choisit l'élément de même texte. */
    node->state.list.selected_index =
        (var->Type == WIDGET_COMBOBOXTEXT && node->state.list.item_count > 0) ? 0 : -1;
    if (!node->initialised) {
        el = NULL;
        text = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        for (i = 0; text && i < node->state.list.item_count; i++) {
            if (strcmp(node->state.list.items[i], text) == 0) {
                node->state.list.selected_index = i;
                break;
            }
        }
        /* <comboboxentry> : un <default> absent de la liste (ou une liste
         * vide) reste le texte du widget, comme dans le champ de saisie de
         * l'étalon. Ce port n'a pas de champ : le texte est gardé et exporté
         * tant que rien n'est choisi. Jusqu'à la 3.3.0 il était perdu. */
        if (text && var->Type == WIDGET_COMBOBOXENTRY && node->state.list.selected_index < 0) {
            /* Le texte entier pour l'export ; filter (255 octets) pour l'affichage.
             * Jusqu'à la 3.3.0 un défaut plus long sortait coupé sur ce port. */
            free(node->texte_libre);
            node->texte_libre = strdup(text);
            snprintf(node->state.list.filter, sizeof(node->state.list.filter), "%s", text);
        }
        node->initialised = TRUE;
    }
    cbt_jouer_si_change(var, node, avant);
}
void widget_comboboxtext_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *node = (WidgetNode *)var->Widget;
    char *avant = strdup(cbt_texte(node));
    int idx = node->state.list.selected_index, i;

    /* Comme l'étalon : l'élément choisi est retiré, puis le précédent est
     * choisi (le premier s'il n'y en a pas). Jusqu'à la 3.3.0 ce port ne
     * faisait rien. */
    if (idx >= 0 && idx < node->state.list.item_count) {
        free(node->state.list.items[idx]);
        for (i = idx; i + 1 < node->state.list.item_count; i++)
            node->state.list.items[i] = node->state.list.items[i + 1];
        node->state.list.item_count--;
    }
    if (var->Type == WIDGET_COMBOBOXENTRY) cbt_vider_texte_libre(node);
    idx = idx > 0 ? idx - 1 : 0;
    node->state.list.selected_index = idx < node->state.list.item_count ? idx : -1;
    cbt_jouer_si_change(var, node, avant);
}
void widget_comboboxtext_fileselect(variable *var, const char *n, const char *v) {}
void widget_comboboxtext_save(variable *var) {}
