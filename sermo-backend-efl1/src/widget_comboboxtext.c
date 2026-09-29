/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_comboboxtext.c — Liste déroulante éditable EFL
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
#include "widget_comboboxtext.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void _cbt_selected(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    const char *label = elm_object_item_text_get(item);
    if (label) {
        free(evas_object_data_get(obj, "text_val"));
        evas_object_data_set(obj, "text_val", strdup(label));
    }
}

/* Pose la sélection : l'étiquette affichée et la valeur exportée ensemble. */
static void _cbt_select(Evas_Object *hs, const char *label)
{
    elm_object_text_set(hs, label);
    free(evas_object_data_get(hs, "text_val"));
    evas_object_data_set(hs, "text_val", strdup(label));
}

GtkWidget *widget_comboboxtext_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *hs = elm_hoversel_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    (void)Attr; (void)attr; (void)Type;
    evas_object_smart_callback_add(hs, "selected", _cbt_selected, NULL);
    evas_object_data_set(hs, "text_val", strdup(""));

    /* <input>, <item> et <default> : chargés par widget_comboboxtext_refresh(),
     * que le cœur appelle juste après la création. Un refresh recharge la
     * liste entière : les éléments posés ici y seraient perdus ou doublés. */
    evas_object_show(hs);
    return (GtkWidget *)hs;
}

gchar *widget_comboboxtext_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("");
    const char *v = (const char *)evas_object_data_get((Evas_Object *)w, "text_val");
    return g_strdup(v ? v : "");
}
gchar *widget_comboboxtext_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_comboboxtext_envvar_construct(var->Widget);
}
void widget_comboboxtext_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_hoversel_clear((Evas_Object *)var->Widget);
}
void widget_comboboxtext_refresh(variable *var)
{
    Evas_Object *hs;
    const Eina_List *l;
    Elm_Object_Item *it;
    GList *el = NULL;
    gchar **lines, **p;
    gchar *item, *def;
    int initialised;

    if (!var || !var->Widget || !var->Attributes) return;
    hs = (Evas_Object *)var->Widget;
    /* Premier passage ou non : notre propre marque. Celle du cœur
     * (« _initialised ») n'existe pas ici : le cœur neutre compile
     * g_object_set_data en no-op (sermocore-shim.h). */
    initialised = evas_object_data_get(hs, "cbt_initialised") != NULL;
    evas_object_data_set(hs, "cbt_initialised", (void *)1);

    /* Étalon gtk3sermo : un refresh RECHARGE la liste, il ne l'allonge pas. */
    if (initialised) {
        elm_hoversel_clear(hs);
        _cbt_select(hs, "");
    }

    /* <input> (commande ou fichier) : chaque ligne, vide comprise, un élément
     * (mesuré sur l'étalon : « \nun\n » exporte ""). Lu ici seulement : lu
     * aussi à la création, la commande tournerait deux fois. */
    lines = widget_input_lines(var->Attributes);
    for (p = lines; p && *p; p++)
        elm_hoversel_item_add(hs, *p, NULL, ELM_ICON_NONE, NULL, NULL);
    g_strfreev(lines);

    /* <item> APRÈS <input> : c'est l'ordre de l'étalon */
    for (item = attributeset_get_first(&el, var->Attributes, ATTR_ITEM); item;
         item = attributeset_get_next(&el, var->Attributes, ATTR_ITEM))
        if (*item)
            elm_hoversel_item_add(hs, item, NULL, ELM_ICON_NONE, NULL, NULL);

    /* comboboxtext : l'élément 0 est sélectionné ; comboboxentry : rien */
    l = elm_hoversel_items_get(hs);
    if (var->Type == WIDGET_COMBOBOXTEXT && l) {
        const char *premier = elm_object_item_text_get(eina_list_data_get(l));
        if (premier) _cbt_select(hs, premier);
    }

    /* <default>, au premier passage seulement : l'élément de même texte. Un
     * défaut absent de la liste ne change rien, sauf pour comboboxentry où il
     * devient le texte saisi — et seulement si la liste n'est pas vide
     * (mesuré sur l'étalon). */
    el = NULL;
    def = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
    if (!initialised && def && elm_hoversel_items_get(hs)) {
        int trouve = 0;
        EINA_LIST_FOREACH(elm_hoversel_items_get(hs), l, it) {
            const char *t = elm_object_item_text_get(it);
            if (t && strcmp(t, def) == 0) {
                _cbt_select(hs, t);
                trouve = 1;
                break;
            }
        }
        if (!trouve && var->Type == WIDGET_COMBOBOXENTRY)
            _cbt_select(hs, def);
    }
}
void widget_comboboxtext_fileselect(variable *var, const char *n, const char *v) {}
void widget_comboboxtext_removeselected(variable *var) {}
void widget_comboboxtext_save(variable *var) {}
