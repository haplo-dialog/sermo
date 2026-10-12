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
#include "sermo_focus.h"
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_tree.h"
#include "safe_exec.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static char *_tree_text_get(void *data, Evas_Object *obj, const char *part)
{
    (void)obj; (void)part;
    return strdup((const char *)data);
}

/* Colonnes « a|b|c » (format de l'étalon) : style « full » de genlist, une
 * rangée d'étiquettes, chacune à la largeur de la plus longue cellule de sa
 * colonne. Jusqu'à la 2.8.0, la ligne brute « a|b|c » s'affichait. */
static Evas_Object *_tree_content_get(void *data, Evas_Object *obj, const char *part)
{
    (void)part;
    int pas = (int)(intptr_t)evas_object_data_get(obj, "pas_colonne");
    /* Une table : dans chaque cellule, un rectangle transparent de la largeur
     * de la colonne, et l'étiquette par-dessus — la largeur minimale posée
     * sur une elm_label seule est ignorée (mesuré : cellules collées). */
    Evas_Object *tb = elm_table_add(obj);
    gchar **c = g_strsplit((const char *)data, "|", -1);
    for (int i = 0; c[i]; i++) {
        Evas_Object *r = evas_object_rectangle_add(evas_object_evas_get(obj));
        evas_object_color_set(r, 0, 0, 0, 0);
        evas_object_size_hint_min_set(r, pas > 0 ? pas : 120, 1);
        elm_table_pack(tb, r, i, 0, 1, 1);
        Evas_Object *l = elm_label_add(tb);
        char *m = elm_entry_utf8_to_markup(c[i]);
        elm_object_text_set(l, m ? m : "");
        free(m);
        evas_object_size_hint_align_set(l, 0.0, 0.5);
        evas_object_show(l);
        elm_table_pack(tb, l, i, 0, 1, 1);
    }
    g_strfreev(c);
    evas_object_size_hint_align_set(tb, 0.0, 0.5);
    evas_object_show(tb);
    return tb;
}

/* Largeur d'une colonne : la plus longue cellule, à ~8 px le caractère. */
static void _tree_mesurer(Evas_Object *gl, const char *t)
{
    int pas = (int)(intptr_t)evas_object_data_get(gl, "pas_colonne");
    gchar **c = g_strsplit(t ? t : "", "|", -1);
    for (int i = 0; c[i]; i++) {
        int w = (int)g_utf8_strlen(c[i], -1) * 8 + 24;
        if (w > pas) pas = w;
    }
    g_strfreev(c);
    evas_object_data_set(gl, "pas_colonne", (void *)(intptr_t)pas);
}

/* Le texte de la rangée, en sautant la ligne d'en-têtes (désactivée). */
static Elm_Object_Item *_tree_premiere(Evas_Object *gl)
{
    Elm_Object_Item *it = elm_genlist_first_item_get(gl);
    while (it && elm_object_item_disabled_get(it)) it = elm_genlist_item_next_get(it);
    return it;
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

static Elm_Genlist_Item_Class *_get_tree_itc_colonnes(void)
{
    static Elm_Genlist_Item_Class *itc = NULL;
    if (!itc) {
        itc = elm_genlist_item_class_new();
        itc->item_style       = "full";
        itc->func.text_get    = NULL;
        itc->func.content_get = _tree_content_get;
        itc->func.state_get   = NULL;
        itc->func.del         = _tree_item_del;
    }
    return itc;
}

/* Une rangée : à colonnes si la ligne porte « | ». */
static Elm_Object_Item *_tree_ajouter(Evas_Object *gl, const char *t)
{
    Elm_Genlist_Item_Class *itc = strchr(t, '|') ? _get_tree_itc_colonnes() : _get_tree_itc();
    return elm_genlist_item_append(gl, itc, strdup(t), NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
}

GtkWidget *widget_tree_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *gl = elm_genlist_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    (void)Attr;
    evas_object_data_set(gl, "selected", strdup(""));
    evas_object_data_set(gl, "sermo-focus", (void *)(intptr_t)sermo_focus_initial("tree"));
    /* <tree exported-column="N"> : la colonne que la variable rend (défaut 0) */
    if (attr) {
        const char *v = get_tag_attribute(attr, "exported-column");
        if (v && atoi(v) > 0) evas_object_data_set(gl, "sermo-colonne", (void *)(intptr_t)atoi(v));
    }

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
    /* Parité gtk3 : sans sélection, la 1re ligne seulement si l'arbre prend
     * le focus à l'ouverture (sermo_focus.h ; jusqu'à la 2.8.1 : toujours) */
    if (!sel && evas_object_data_get((Evas_Object *)w, "sermo-focus"))
        sel = _tree_premiere((Evas_Object *)w);
    if (!sel) return g_strdup("");
    /* genlist : le texte vit dans la DONNEE de l'item (text_get est un
     * callback, elm_object_item_text_get rend NULL) */
    txt = (const char *)elm_object_item_data_get(sel);
    if (!txt) txt = elm_object_item_text_get(sel);
    if (!txt) return g_strdup("");
    /* la rangée garde sa ligne entière ; seule la colonne « exported-column »
     * est exportée, comme l'étalon (jusqu'à la 2.8.1 : toujours la 1re) */
    for (intptr_t n = (intptr_t)evas_object_data_get((Evas_Object *)w, "sermo-colonne"); n > 0; n--) {
        txt = strchr(txt, '|');
        if (!txt) return g_strdup("");
        txt++;
    }
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
    GList *el = NULL;
    gchar **lines, **p;
    gchar *item;

    if (!var || !var->Widget || !var->Attributes) return;
    gl = (Evas_Object *)var->Widget;

    /* Étalon gtk3sermo : chaque refresh vide l'arbre puis recharge tout. */
    elm_genlist_clear(gl);
    evas_object_data_set(gl, "pas_colonne", NULL);

    /* <label>Col1|Col2</label> : une ligne d'en-têtes, qui ne se choisit pas */
    {
        GList *e2 = NULL;
        gchar *lbl = attributeset_get_first(&e2, var->Attributes, ATTR_LABEL);
        if (lbl && strchr(lbl, '|')) {
            _tree_mesurer(gl, lbl);
            Elm_Object_Item *h = _tree_ajouter(gl, lbl);
            if (h) elm_object_item_disabled_set(h, EINA_TRUE);
        }
    }

    /* <input> (commande ou fichier) : chaque ligne, vide comprise, une rangée.
     * Lu ici seulement : lu aussi à la création, la commande tournerait deux
     * fois. */
    lines = widget_input_lines(var->Attributes);
    for (p = lines; p && *p; p++) {
        _tree_mesurer(gl, *p);
        _tree_ajouter(gl, *p);
    }
    g_strfreev(lines);

    /* <item> APRÈS <input>, dans l'ordre de l'étalon */
    for (item = attributeset_get_first(&el, var->Attributes, ATTR_ITEM); item;
         item = attributeset_get_next(&el, var->Attributes, ATTR_ITEM))
        if (*item) {
            _tree_mesurer(gl, item);
            _tree_ajouter(gl, item);
        }
}
void widget_tree_fileselect(variable *var, const char *n, const char *v) {}
void widget_tree_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    Elm_Object_Item *sel = elm_genlist_selected_item_get((Evas_Object *)var->Widget);
    if (sel) elm_object_item_del(sel);
}
void widget_tree_save(variable *var) {}
