/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_overlay.c — Enfants empilés (EFL/Elementary)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Une elm_table où TOUS les enfants occupent la même cellule (0,0) : ils se
 * superposent, dans l'ordre d'empaquetage. Le premier est le fond, comme
 * GtkOverlay. */
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
#include "widget_overlay.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

GtkWidget *widget_overlay_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) attr; (void) Type;

    Evas_Object *parent = efl_main_win_get();
    Evas_Object *table = elm_table_add(parent ? parent
                                              : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
    int poses = 0;

    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n) {
        Evas_Object *c = (Evas_Object *) s.widgets[n];
        if (!c) continue;
        evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(c, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_table_pack(table, c, 0, 0, 1, 1);   /* même cellule = superposition */
        evas_object_show(c);
        poses++;
    }
    if (poses < 2)
        fprintf(stderr, "efl1sermo: <overlay> n'a reçu que %d enfant(s) : il n'y "
                        "a rien à superposer.\n", poses);

    evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(table);
    return (GtkWidget *) table;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_overlay_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_overlay_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_overlay_envvar_construct(var->Widget);
}
void widget_overlay_clear(variable *var)          { (void) var; }
void widget_overlay_refresh(variable *var)        { (void) var; }
void widget_overlay_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_overlay_removeselected(variable *var) { (void) var; }
void widget_overlay_save(variable *var)           { (void) var; }
