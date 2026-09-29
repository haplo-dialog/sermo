/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_list.c — Liste EFL (elm_genlist simple)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_list.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Item class simple pour elm_genlist */
static char *_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
    return strdup((const char *)data);
}

/* Chaque rangée porte une copie strdup() de son texte : la libérer quand la
 * rangée disparaît. Sans elle, chaque clear: ou refresh: perdait toutes les
 * copies — une liste rechargée en boucle grossissait sans fin. */
static void _gl_item_del(void *data, Evas_Object *obj)
{
    (void)obj;
    free(data);
}

static Elm_Genlist_Item_Class *_get_item_class(void)
{
    static Elm_Genlist_Item_Class *itc = NULL;
    if (!itc) {
        itc = elm_genlist_item_class_new();
        itc->item_style     = "default";
        itc->func.text_get  = _gl_text_get;
        itc->func.content_get = NULL;
        itc->func.state_get = NULL;
        itc->func.del       = _gl_item_del;
    }
    return itc;
}

static void _list_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    const char *txt = elm_object_item_text_get(item);
    /* Construire la sélection (accumulation séparée par |) */
    char *cur = (char *)evas_object_data_get(obj, "selected");
    if (cur && *cur) {
        size_t nl = strlen(cur)+strlen(txt)+2;
        char *ns  = malloc(nl);
        snprintf(ns, nl, "%s|%s", cur, txt);
        free(cur);
        evas_object_data_set(obj, "selected", ns);
    } else {
        free(cur);
        evas_object_data_set(obj, "selected", strdup(txt));
    }
}

GtkWidget *widget_list_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *gl = elm_genlist_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    elm_genlist_multi_select_set(gl, EINA_TRUE);
    evas_object_data_set(gl, "selected", strdup(""));
    evas_object_smart_callback_add(gl, "selected", _list_sel, NULL);

    Elm_Genlist_Item_Class *itc = _get_item_class();

    if (Attr) {
        GList *el = NULL;
        gchar *item = attributeset_get_first(&el, Attr, ATTR_ITEM);
        gchar *premier = NULL;
        while (item) {
            if (*item) {
                if (!premier) premier = item;
                elm_genlist_item_append(gl, itc, strdup(item), NULL,
                                       ELM_GENLIST_ITEM_NONE, NULL, NULL);
            }
            item = attributeset_get_next(&el, Attr, ATTR_ITEM);
        }
        /* Parite etalon : sans <default>, le PREMIER item est selectionne */
        if (premier) {
            free(evas_object_data_get(gl, "selected"));
            evas_object_data_set(gl, "selected", strdup(premier));
        }
        /* <input> : lu par widget_list_refresh(), que le cœur appelle juste
         * après la création (lu ici aussi, la commande tournerait deux fois). */
    }
    evas_object_show(gl);
    return (GtkWidget *)gl;
}

gchar *widget_list_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("");
    const char *sel = (const char *)evas_object_data_get((Evas_Object *)w, "selected");
    return g_strdup(sel ? sel : "");
}
gchar *widget_list_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_list_envvar_construct(var->Widget);
}
void widget_list_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_genlist_clear((Evas_Object *)var->Widget);
    free(evas_object_data_get((Evas_Object *)var->Widget, "selected"));
    evas_object_data_set((Evas_Object *)var->Widget, "selected", strdup(""));
}
void widget_list_refresh(variable *var)
{
    Evas_Object *gl;
    gchar **lines, **p;
    int vide;

    if (!var || !var->Widget || !var->Attributes) return;
    gl = (Evas_Object *)var->Widget;
    /* <input> (commande ou fichier) : chaque ligne, vide comprise, AJOUTE une
     * rangée — l'étalon gtk3sermo ne vide pas une liste au refresh. */
    lines = widget_input_lines(var->Attributes);
    if (!lines) return;
    vide = elm_genlist_first_item_get(gl) == NULL;
    for (p = lines; *p; p++)
        elm_genlist_item_append(gl, _get_item_class(), strdup(*p), NULL,
                                ELM_GENLIST_ITEM_NONE, NULL, NULL);
    /* Comme après <item> : une liste jusque-là vide exporte sa 1re rangée. */
    if (vide && lines[0]) {
        free(evas_object_data_get(gl, "selected"));
        evas_object_data_set(gl, "selected", strdup(lines[0]));
    }
    g_strfreev(lines);
}
void widget_list_fileselect(variable *var, const char *n, const char *v) {}
void widget_list_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    Evas_Object *gl = (Evas_Object *)var->Widget;
    Elm_Object_Item *sel = elm_genlist_selected_item_get(gl);
    if (sel) elm_object_item_del(sel);
}
void widget_list_save(variable *var) {}
