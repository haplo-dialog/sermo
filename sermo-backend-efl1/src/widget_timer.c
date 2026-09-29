/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_timer.c — Minuterie EFL (ecore_timer)
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
#include "widget_timer.h"
#include "safe_exec.h"
#include "actions.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    Evas_Object  *placeholder;
    long long     ticks;
    AttributeSet *Attr;      /* actions iterees a chaque tick */
    Eina_Bool     enabled;   /* exporte "true"/"false" (parite etalon) */
} TimerData;

static Eina_Bool _timer_cb(void *data)
{
    TimerData *td = (TimerData *)data;
    td->ticks++;
    /* TOUTES les actions, via le repartiteur du coeur (exit:, refresh:,
     * shell...) — safe_system direct traitait « exit:fin » comme une
     * commande et le dialogue ne se fermait jamais. */
    if (td->Attr) {
        GList *el = NULL;
        gchar *fn = attributeset_get_first(&el, td->Attr, ATTR_ACTION);
        while (fn) {
            if (*fn) execute_action((GtkWidget *)td->placeholder, fn, NULL);
            fn = attributeset_get_next(&el, td->Attr, ATTR_ACTION);
        }
    }
    return ECORE_CALLBACK_RENEW;
}

GtkWidget *widget_timer_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    /* Widget invisible porteur des données */
    Evas_Object *lbl = elm_label_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    evas_object_hide(lbl);

    double interval = 1.0;
    if (attr) {
        const char *v;
        /* g_ascii_strtod, pas atof : sous locale fr, atof("0.5") rend 0. */
        if ((v = get_tag_attribute(attr, "interval")))
            interval = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "milliseconds")))
            interval = g_ascii_strtod(v, NULL) / 1000.0;
        if (interval <= 0) interval = 1.0;
    }

    TimerData *td = calloc(1, sizeof(TimerData));
    td->placeholder = lbl;
    td->ticks   = 0;
    td->Attr    = Attr;
    td->enabled = EINA_TRUE;

    evas_object_data_set(lbl, "timer_data", td);
    ecore_timer_add(interval, _timer_cb, td);

    return (GtkWidget *)lbl;
}

gchar *widget_timer_envvar_construct(GtkWidget *w)
{
    /* Parite de VALEUR avec l'etalon : un timer exporte son etat
     * "true"/"false", pas un compteur de ticks. */
    TimerData *td;
    if (!w) return g_strdup("false");
    td = (TimerData *)evas_object_data_get((Evas_Object *)w, "timer_data");
    if (!td) return g_strdup("false");
    return g_strdup(td->enabled ? "true" : "false");
}
gchar *widget_timer_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_timer_envvar_construct(var->Widget);
}
void widget_timer_clear(variable *var)
{
    if (!var || !var->Widget) return;
    TimerData *td = (TimerData *)evas_object_data_get((Evas_Object *)var->Widget, "timer_data");
    if (td) { td->ticks = 0; td->enabled = EINA_FALSE; }
}
void widget_timer_refresh(variable *var) {}
void widget_timer_fileselect(variable *var, const char *n, const char *v) {}
void widget_timer_removeselected(variable *var) {}
void widget_timer_save(variable *var) {}
