/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_flowbox.c — Rangement en lignes (EFL/Elementary)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ PAS elm_gengrid : il range des ITEMS construits par une classe de rappel,
 * pas des widgets quelconques — or <flowbox> contient ce que le script y met.
 * Une elm_table à « max-children-per-line » colonnes donne le même rendu, sans
 * reflux dynamique (limite du port, pas du tag).
 *
 * ⚠️ Pas de sélection : l'export rend toujours la chaîne vide. */
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
#include "widget_flowbox.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

GtkWidget *widget_flowbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    Evas_Object *parent = efl_main_win_get();
    Evas_Object *table = elm_table_add(parent ? parent
                                              : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
    int par_ligne = 4, esp_col = 6, esp_lig = 6;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "max-children-per-line"))) par_ligne = atoi(v);
        if ((v = get_tag_attribute(attr, "column-spacing")))        esp_col   = atoi(v);
        if ((v = get_tag_attribute(attr, "row-spacing")))           esp_lig   = atoi(v);
    }
    if (par_ligne < 1) par_ligne = 1;
    elm_table_padding_set(table, esp_col, esp_lig);

    stackelement s = pop();
    int i = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        Evas_Object *c = (Evas_Object *) s.widgets[n];
        if (!c) continue;
        evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(c, EVAS_HINT_FILL, 0.5);
        elm_table_pack(table, c, i % par_ligne, i / par_ligne, 1, 1);
        evas_object_show(c);
        i++;
    }
    evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(table, EVAS_HINT_FILL, 0.5);
    evas_object_show(table);
    return (GtkWidget *) table;
}

/* Export : l'index de l'enfant sélectionné — ce port n'en sélectionne aucun. */
gchar *widget_flowbox_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_flowbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_flowbox_envvar_construct(var->Widget);
}
void widget_flowbox_clear(variable *var)          { (void) var; }
void widget_flowbox_refresh(variable *var)        { (void) var; }
void widget_flowbox_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_flowbox_removeselected(variable *var) { (void) var; }
void widget_flowbox_save(variable *var)           { (void) var; }
