/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_toolbar.c — Barre d'actions (EFL/Elementary)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ PAS elm_toolbar. Il existe, mais il range des ITEMS (elm_toolbar_item_append,
 * une icône et un libellé), pas des widgets quelconques — or <toolbar> contient
 * ce que le script y met : une case à cocher, un champ de saisie, un bouton.
 * Une elm_box horizontale reçoit tout ça, comme <hbox> sur ce port.
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
#include "widget_toolbar.h"
#include <stdlib.h>

GtkWidget *widget_toolbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    Evas_Object *parent = efl_main_win_get();
    Evas_Object *barre = elm_box_add(parent ? parent
                                            : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
    int vertical = 0, espacement = 4;

    if (attr) {
        const char *v = get_tag_attribute(attr, "orientation");
        if (v && !strcasecmp(v, "vertical")) vertical = 1;
        if ((v = get_tag_attribute(attr, "spacing"))) espacement = atoi(v);
    }
    elm_box_horizontal_set(barre, vertical ? EINA_FALSE : EINA_TRUE);
    elm_box_padding_set(barre, vertical ? 0 : espacement, vertical ? espacement : 0);
    elm_box_align_set(barre, 0.0, 0.5);

    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n) {
        Evas_Object *c = (Evas_Object *) s.widgets[n];
        if (!c) continue;
        evas_object_size_hint_weight_set(c, 0.0, 0.0);
        evas_object_size_hint_align_set(c, 0.0, EVAS_HINT_FILL);
        elm_box_pack_end(barre, c);
        evas_object_show(c);
    }

    evas_object_size_hint_weight_set(barre, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(barre, EVAS_HINT_FILL, 0.0);
    evas_object_show(barre);
    return (GtkWidget *) barre;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_toolbar_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_toolbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_toolbar_envvar_construct(var->Widget);
}
void widget_toolbar_clear(variable *var)          { (void) var; }
void widget_toolbar_refresh(variable *var)        { (void) var; }
void widget_toolbar_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_toolbar_removeselected(variable *var) { (void) var; }
void widget_toolbar_save(variable *var)           { (void) var; }
