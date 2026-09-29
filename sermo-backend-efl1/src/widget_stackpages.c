/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_stackpages.c — N pages, une seule visible (EFL/Elementary)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ PAS elm_naviframe. Il existe (elc_naviframe.h) mais c'est une pile à
 * EMPILER/DÉPILER (push/pop), pas un jeu de pages adressables par index : y
 * aller directement demanderait de tout re-pousser. Une elm_box où l'on ne
 * MONTRE qu'un enfant donne exactement <stack>, et l'index reste trivial.
 *
 * L'index vit dans les données de l'objet : création et export sont dans deux
 * fichiers différents (leçon du 2026-09-14).
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
#include "stack.h"
#include "widget_stackpages.h"
#include <stdlib.h>
#include <stdint.h>

/* Montre la page voulue, cache les autres. */
static void pages_aller_a(Evas_Object *pile, int page)
{
    Eina_List *enfants = elm_box_children_get(pile), *l;
    Evas_Object *c;
    int i = 0, total = eina_list_count(enfants);

    if (page < 0) page = 0;
    if (page >= total && total > 0) page = total - 1;
    EINA_LIST_FOREACH(enfants, l, c) {
        if (i == page) evas_object_show(c);
        else           evas_object_hide(c);
        i++;
    }
    eina_list_free(enfants);
    evas_object_data_set(pile, "sermo_page", (void *)(intptr_t) page);
}

static void bouton_page(void *data, Evas_Object *obj, void *ev)
{
    Evas_Object *pile = (Evas_Object *) data;
    (void) ev;
    pages_aller_a(pile, (int)(intptr_t) evas_object_data_get(obj, "sermo_index"));
}

GtkWidget *widget_stackpages_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    Evas_Object *parent = efl_main_win_get();
    Evas_Object *racine = parent ? parent : elm_win_add(NULL, "tmp", ELM_WIN_BASIC);
    Evas_Object *pile = elm_box_add(racine);
    int page = 0, avec_switcher = 0, n = 0;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "page"))) page = atoi(v);
        if ((v = get_tag_attribute(attr, "switcher")))
            avec_switcher = (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || atoi(v) == 1);
    }
    elm_box_horizontal_set(pile, EINA_FALSE);

    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i) {
        Evas_Object *c = (Evas_Object *) s.widgets[i];
        if (!c) continue;
        evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(c, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(pile, c);
        n++;
    }
    pages_aller_a(pile, page);
    evas_object_size_hint_weight_set(pile, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(pile, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(pile);

    if (!avec_switcher) return (GtkWidget *) pile;

    {
        Evas_Object *hote  = elm_box_add(racine);
        Evas_Object *barre = elm_box_add(racine);
        elm_box_horizontal_set(hote, EINA_FALSE);
        elm_box_horizontal_set(barre, EINA_TRUE);
        for (int i = 0; i < n; ++i) {
            char t[8];
            Evas_Object *b = elm_button_add(racine);
            snprintf(t, sizeof(t), "%d", i + 1);
            elm_object_text_set(b, t);
            evas_object_data_set(b, "sermo_index", (void *)(intptr_t) i);
            evas_object_smart_callback_add(b, "clicked", bouton_page, pile);
            elm_box_pack_end(barre, b);
            evas_object_show(b);
        }
        evas_object_show(barre);
        elm_box_pack_end(hote, barre);
        elm_box_pack_end(hote, pile);
        evas_object_data_set(hote, "sermo_pile", pile);
        evas_object_size_hint_weight_set(hote, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(hote, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_show(hote);
        return (GtkWidget *) hote;
    }
}

/* Export : l'index de la page visible, comme <notebook> (étalon gtk3). */
gchar *widget_stackpages_envvar_construct(GtkWidget *widget)
{
    Evas_Object *o = (Evas_Object *) widget, *pile;
    char buf[16];

    if (!o) return g_strdup("0");
    pile = (Evas_Object *) evas_object_data_get(o, "sermo_pile");
    if (!pile) pile = o;
    snprintf(buf, sizeof(buf), "%d",
             (int)(intptr_t) evas_object_data_get(pile, "sermo_page"));
    return g_strdup(buf);
}
gchar *widget_stackpages_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_stackpages_envvar_construct(var->Widget);
}
void widget_stackpages_clear(variable *var)          { (void) var; }
void widget_stackpages_refresh(variable *var)        { (void) var; }
void widget_stackpages_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_stackpages_removeselected(variable *var) { (void) var; }
void widget_stackpages_save(variable *var)           { (void) var; }
