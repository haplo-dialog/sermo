/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_radiobutton.c — Bouton radio EFL (elm_radio)
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
#include "widget_radiobutton.h"

extern GtkWidget *lastradiowidget;   /* groupe radio ouvert (cœur, automaton.c) */
#include <string.h>
#include <stdlib.h>

/* Groupe radio global — simplifié : tous les radios dans une même fenêtre partagent le groupe */
static int          _radio_val   = 0;

GtkWidget *widget_radiobutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *rd = elm_radio_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) elm_object_text_set(rd, lbl);
    }

    elm_radio_state_value_set(rd, _radio_val++);

    /* Un groupe par suite de boutons radio, comme gtk3sermo : le cœur ouvre
     * le groupe (lastradiowidget) et le ferme dès qu'un autre widget est créé.
     * Jusqu'à la 2.8.1, efl1 mettait tous les boutons radio de la fenêtre dans
     * un seul groupe : cocher l'un décochait ceux d'une autre question. Le 1er
     * bouton d'un groupe est coché d'office, comme GtkRadioButton. */
    if (lastradiowidget == NULL) {
        lastradiowidget = (GtkWidget *)rd;
        elm_radio_value_set(rd, elm_radio_state_value_get(rd));
    } else {
        elm_radio_group_add(rd, (Evas_Object *)lastradiowidget);
    }

    /* Active si default=true */
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (strcasecmp(def,"true")==0 || strcmp(def,"1")==0))
            elm_radio_value_set(rd, elm_radio_state_value_get(rd));
    }

    evas_object_show(rd);
    return (GtkWidget *)rd;
}

gchar *widget_radiobutton_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("false");
    int grp_val  = elm_radio_value_get((Evas_Object *)w);   /* la valeur de SON groupe */
    int self_val = elm_radio_state_value_get((Evas_Object *)w);
    return g_strdup(grp_val == self_val ? "true" : "false");
}
gchar *widget_radiobutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_radiobutton_envvar_construct(var->Widget);
}
void widget_radiobutton_clear(variable *var) {}
void widget_radiobutton_refresh(variable *var) {}
void widget_radiobutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_radiobutton_removeselected(variable *var) {}
void widget_radiobutton_save(variable *var) {}
