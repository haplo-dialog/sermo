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
#include "jouer_actions.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void _cbt_selected(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    const char *label = elm_object_item_text_get(item);
    if (label) {
        char *avant = evas_object_data_get(obj, "text_val");
        int change = !avant || strcmp(avant, label) != 0;
        free(avant);
        evas_object_data_set(obj, "text_val", strdup(label));
        /* choisir un autre élément joue les <action> de la liste (signal
         * « changed » de l'étalon) ; muettes sur ce port jusqu'à la 3.3.0 */
        if (change)
            jouer_actions((GtkWidget *)obj, (AttributeSet *)data, "changed", NULL);
    }
}

/* Pose la sélection : l'étiquette affichée et la valeur exportée ensemble. */
static void _cbt_select(Evas_Object *hs, const char *label)
{
    elm_object_text_set(hs, label);
    free(evas_object_data_get(hs, "text_val"));
    evas_object_data_set(hs, "text_val", strdup(label));
}

static char *_cbt_avant(Evas_Object *hs)
{
    const char *v = (const char *)evas_object_data_get(hs, "text_val");
    return strdup(v ? v : "");
}

/* Règle de l'étalon gtk3sermo : les <action> jouent quand le texte du widget
 * n'est plus le même après qu'avant. avant : copie libérée ici. */
static void _cbt_jouer_si_change(variable *var, Evas_Object *hs, char *avant)
{
    const char *v = (const char *)evas_object_data_get(hs, "text_val");
    int change = avant && strcmp(avant, v ? v : "") != 0;
    free(avant);
    if (change)
        jouer_actions((GtkWidget *)hs, var->Attributes, "changed", NULL);
}

GtkWidget *widget_comboboxtext_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *hs = elm_hoversel_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    (void)attr; (void)Type;
    evas_object_smart_callback_add(hs, "selected", _cbt_selected, Attr);
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
    Evas_Object *hs;
    char *avant;
    if (!var || !var->Widget) return;
    hs = (Evas_Object *)var->Widget;
    avant = _cbt_avant(hs);
    /* Comme l'étalon : la liste est vidée et plus rien n'est choisi. Jusqu'à
     * la 3.3.0 ce port gardait le texte du dernier choix. */
    elm_hoversel_clear(hs);
    _cbt_select(hs, "");
    _cbt_jouer_si_change(var, hs, avant);
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
    char *avant = NULL;

    if (!var || !var->Widget || !var->Attributes) return;
    hs = (Evas_Object *)var->Widget;
    /* Premier passage ou non : notre propre marque. Celle du cœur
     * (« _initialised ») n'existe pas ici : le cœur neutre compile
     * g_object_set_data en no-op (sermocore-shim.h). */
    initialised = evas_object_data_get(hs, "cbt_initialised") != NULL;
    evas_object_data_set(hs, "cbt_initialised", (void *)1);

    /* Étalon gtk3sermo : un refresh RECHARGE la liste, il ne l'allonge pas. */
    if (initialised) {
        avant = _cbt_avant(hs);   /* rien au premier chargement, comme l'étalon */
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
     * devient le texte du widget, liste vide comprise (l'étalon le perdait
     * sur une liste vide jusqu'à la 3.3.0). */
    el = NULL;
    def = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
    if (!initialised && def) {
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
    _cbt_jouer_si_change(var, hs, avant);
}
void widget_comboboxtext_fileselect(variable *var, const char *n, const char *v) {}
void widget_comboboxtext_removeselected(variable *var)
{
    Evas_Object *hs;
    const Eina_List *l;
    Elm_Object_Item *it, *choisi = NULL;
    char *avant;
    int idx = -1, i = 0;

    if (!var || !var->Widget) return;
    hs = (Evas_Object *)var->Widget;
    avant = _cbt_avant(hs);
    /* Comme l'étalon : l'élément choisi est retiré, puis le précédent est
     * choisi (le premier s'il n'y en a pas). Sans effet jusqu'à la 3.3.0. */
    EINA_LIST_FOREACH(elm_hoversel_items_get(hs), l, it) {
        const char *t = elm_object_item_text_get(it);
        if (*avant && t && strcmp(t, avant) == 0) { choisi = it; idx = i; break; }
        i++;
    }
    if (choisi) elm_object_item_del(choisi);
    idx = idx > 0 ? idx - 1 : 0;
    it = eina_list_nth(elm_hoversel_items_get(hs), idx);
    _cbt_select(hs, it && elm_object_item_text_get(it) ? elm_object_item_text_get(it) : "");
    _cbt_jouer_si_change(var, hs, avant);
}
void widget_comboboxtext_save(variable *var) {}
