/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_tree.c — Arbre EFL (elm_genlist hiérarchique)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <input> et <item> : une ligne, une rangée ; « | » sépare les colonnes et la
 * 1re colonne est la valeur exportée (étalon gtk3sermo). Rangées à plat : la
 * lecture « parent|enfant » d'autrefois n'existait pas chez l'étalon.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
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

static char *_tree_text_get(void *data, Evas_Object *obj, const char *part)
{
    return strdup((const char *)data);
}

/* Chaque rangée porte une copie strdup() de son texte : la libérer quand la
 * rangée disparaît (chaque refresh vide l'arbre avant de le recharger). */
static void _tree_item_del(void *data, Evas_Object *obj)
{
    (void)obj;
    free(data);
}

static Elm_Genlist_Item_Class *_get_tree_itc(void)
{
    static Elm_Genlist_Item_Class *itc = NULL;
    if (!itc) {
        itc = elm_genlist_item_class_new();
        itc->item_style    = "default";
        itc->func.text_get = _tree_text_get;
        itc->func.content_get = NULL;
        itc->func.state_get   = NULL;
        itc->func.del         = _tree_item_del;
    }
    return itc;
}

GtkWidget *widget_tree_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *gl = elm_genlist_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    (void)Attr;
    evas_object_data_set(gl, "selected", strdup(""));

    /* <input> et <item> : chargés par widget_tree_refresh(), que le cœur
     * appelle juste après la création. Ce refresh VIDE l'arbre d'abord
     * (étalon) : des rangées posées ici y disparaîtraient. */
    evas_object_show(gl);
    return (GtkWidget *)gl;
}

gchar *widget_tree_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("");
    Elm_Object_Item *sel = elm_genlist_selected_item_get((Evas_Object *)w);
    const char *txt;
    /* Parite etalon : sans selection, la PREMIERE ligne fait foi */
    if (!sel) sel = elm_genlist_first_item_get((Evas_Object *)w);
    if (!sel) return g_strdup("");
    /* genlist : le texte vit dans la DONNEE de l'item (text_get est un
     * callback, elm_object_item_text_get rend NULL) */
    txt = (const char *)elm_object_item_data_get(sel);
    if (!txt) txt = elm_object_item_text_get(sel);
    if (!txt) return g_strdup("");
    /* la rangée garde sa ligne entière ; seule la 1re colonne est exportée */
    return g_strndup(txt, strcspn(txt, "|"));
}
gchar *widget_tree_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_tree_envvar_construct(var->Widget);
}
void widget_tree_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_genlist_clear((Evas_Object *)var->Widget);
}
void widget_tree_refresh(variable *var)
{
    Evas_Object *gl;
    Elm_Genlist_Item_Class *itc = _get_tree_itc();
    GList *el = NULL;
    gchar **lines, **p;
    gchar *item;

    if (!var || !var->Widget || !var->Attributes) return;
    gl = (Evas_Object *)var->Widget;

    /* Étalon gtk3sermo : chaque refresh vide l'arbre puis recharge tout. */
    elm_genlist_clear(gl);

    /* <input> (commande ou fichier) : chaque ligne, vide comprise, une rangée.
     * Lu ici seulement : lu aussi à la création, la commande tournerait deux
     * fois. */
    lines = widget_input_lines(var->Attributes);
    for (p = lines; p && *p; p++)
        elm_genlist_item_append(gl, itc, strdup(*p), NULL,
                                ELM_GENLIST_ITEM_NONE, NULL, NULL);
    g_strfreev(lines);

    /* <item> APRÈS <input>, dans l'ordre de l'étalon */
    for (item = attributeset_get_first(&el, var->Attributes, ATTR_ITEM); item;
         item = attributeset_get_next(&el, var->Attributes, ATTR_ITEM))
        if (*item)
            elm_genlist_item_append(gl, itc, strdup(item), NULL,
                                    ELM_GENLIST_ITEM_NONE, NULL, NULL);
}
void widget_tree_fileselect(variable *var, const char *n, const char *v) {}
void widget_tree_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    Elm_Object_Item *sel = elm_genlist_selected_item_get((Evas_Object *)var->Widget);
    if (sel) elm_object_item_del(sel);
}
void widget_tree_save(variable *var) {}
