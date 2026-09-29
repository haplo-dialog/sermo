/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_wizard.c — Suite d'étapes (EFL/Elementary)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Même montage que <stack> (une elm_box où l'on ne MONTRE qu'un enfant), plus
 * trois boutons. L'index vit dans les données de l'objet : création et export
 * sont dans deux fichiers (leçon du 2026-09-14).
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
#include "actions.h"
#include "widget_wizard.h"
#include <stdlib.h>
#include <stdint.h>

static void wiz_montrer(Evas_Object *pile, int etape)
{
    Eina_List *enfants = elm_box_children_get(pile), *l;
    Evas_Object *c;
    int i = 0, total = eina_list_count(enfants);

    if (etape < 0) etape = 0;
    if (etape >= total && total > 0) etape = total - 1;
    EINA_LIST_FOREACH(enfants, l, c) {
        if (i == etape) evas_object_show(c);
        else            evas_object_hide(c);
        i++;
    }
    eina_list_free(enfants);
    evas_object_data_set(pile, "sermo_etape", (void *)(intptr_t) etape);
}

static void wiz_precedent(void *data, Evas_Object *o, void *ev)
{
    Evas_Object *pile = (Evas_Object *) data;
    (void) o; (void) ev;
    wiz_montrer(pile, (int)(intptr_t) evas_object_data_get(pile, "sermo_etape") - 1);
}
static void wiz_suivant(void *data, Evas_Object *o, void *ev)
{
    Evas_Object *pile = (Evas_Object *) data;
    (void) o; (void) ev;
    wiz_montrer(pile, (int)(intptr_t) evas_object_data_get(pile, "sermo_etape") + 1);
}
static void wiz_terminer(void *data, Evas_Object *o, void *ev)
{
    AttributeSet *Attr = (AttributeSet *) data;
    GList *el = NULL;
    gchar *cmd;
    (void) ev;
    if (!Attr) return;
    cmd = attributeset_get_first(&el, Attr, ATTR_ACTION);
    while (cmd) {
        if (*cmd) execute_action((GtkWidget *) o, cmd, NULL);
        cmd = attributeset_get_next(&el, Attr, ATTR_ACTION);
    }
}

GtkWidget *widget_wizard_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) attr; (void) Type;

    Evas_Object *parent = efl_main_win_get();
    Evas_Object *racine = parent ? parent : elm_win_add(NULL, "tmp", ELM_WIN_BASIC);
    Evas_Object *pile  = elm_box_add(racine);
    Evas_Object *hote  = elm_box_add(racine);
    Evas_Object *barre = elm_box_add(racine);
    Evas_Object *prec, *suiv, *fin;

    elm_box_horizontal_set(pile, EINA_FALSE);
    elm_box_horizontal_set(hote, EINA_FALSE);
    elm_box_horizontal_set(barre, EINA_TRUE);

    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i) {
        Evas_Object *c = (Evas_Object *) s.widgets[i];
        if (!c) continue;
        evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(c, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(pile, c);
    }
    wiz_montrer(pile, 0);

    prec = elm_button_add(racine); elm_object_text_set(prec, "Précédent");
    suiv = elm_button_add(racine); elm_object_text_set(suiv, "Suivant");
    fin  = elm_button_add(racine); elm_object_text_set(fin,  "Terminer");
    evas_object_smart_callback_add(prec, "clicked", wiz_precedent, pile);
    evas_object_smart_callback_add(suiv, "clicked", wiz_suivant, pile);
    evas_object_smart_callback_add(fin,  "clicked", wiz_terminer, Attr);
    elm_box_pack_end(barre, prec); evas_object_show(prec);
    elm_box_pack_end(barre, suiv); evas_object_show(suiv);
    elm_box_pack_end(barre, fin);  evas_object_show(fin);

    evas_object_size_hint_weight_set(pile, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(pile, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(pile); evas_object_show(barre);
    elm_box_pack_end(hote, pile);
    elm_box_pack_end(hote, barre);
    evas_object_data_set(hote, "sermo_pile", pile);
    evas_object_size_hint_weight_set(hote, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(hote, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(hote);
    return (GtkWidget *) hote;
}

/* Export : l'index de l'étape courante, comme <stack>. */
gchar *widget_wizard_envvar_construct(GtkWidget *widget)
{
    Evas_Object *o = (Evas_Object *) widget, *pile;
    char buf[16];

    if (!o) return g_strdup("0");
    pile = (Evas_Object *) evas_object_data_get(o, "sermo_pile");
    if (!pile) pile = o;
    snprintf(buf, sizeof(buf), "%d",
             (int)(intptr_t) evas_object_data_get(pile, "sermo_etape"));
    return g_strdup(buf);
}
gchar *widget_wizard_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_wizard_envvar_construct(var->Widget);
}
void widget_wizard_clear(variable *var)          { (void) var; }
void widget_wizard_refresh(variable *var)        { (void) var; }
void widget_wizard_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_wizard_removeselected(variable *var) { (void) var; }
void widget_wizard_save(variable *var)           { (void) var; }
