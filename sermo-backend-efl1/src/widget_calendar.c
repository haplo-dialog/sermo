/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_calendar.c — Sélecteur de date EFL (elm_calendar)
 * sermo — haplo-dialog — GPL-2.0-or-later
 * Export : "YYYY-MM-DD"
 *
 * elm_calendar_selected_time_set()/_get() ne font PAS un aller-retour fidèle :
 * l'heure passée est ignorée et, si la date visée n'est pas dans la même saison
 * (heure d'été / heure d'hiver) que « maintenant », la date lue recule d'un jour
 * (mktime interne appliqué avec le décalage UTC courant). On ne se fie donc pas
 * à ce round-trip pour la valeur exportée : on garde la date canonique dans une
 * chaîne attachée au widget, mise à jour par le rappel « changed » quand
 * l'utilisateur choisit une date (le clic pose directement le jour, sans le
 * décalage). Voir tests/comportement/cas/10-calendar. */
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
#include "widget_calendar.h"
#include <Elementary.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define CAL_DATE_KEY "sermo_cal_date"

static void cal_store_date(Evas_Object *cal, int y, int m, int d)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
    char *old = evas_object_data_del(cal, CAL_DATE_KEY);
    if (old) g_free(old);
    evas_object_data_set(cal, CAL_DATE_KEY, g_strdup(buf));
}

/* L'utilisateur a choisi une date : le jour sélectionné est fiable ici. */
static void cal_changed_cb(void *data, Evas_Object *obj, void *einfo)
{
    (void)data; (void)einfo;
    struct tm t = {0};
    elm_calendar_selected_time_get(obj, &t);
    cal_store_date(obj, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
}

GtkWidget *widget_calendar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *cal = elm_calendar_add(win);

    /* Date par défaut = aujourd'hui, sauf <default> explicite. */
    time_t now = time(NULL);
    struct tm nt; localtime_r(&now, &nt);
    int y = nt.tm_year + 1900, m = nt.tm_mon + 1, d = nt.tm_mday;

    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && strlen(def) >= 10) {
            int py=0, pm=0, pd=0;
            if (sscanf(def, "%d-%d-%d", &py, &pm, &pd) == 3) { y=py; m=pm; d=pd; }
        }
    }

    /* Valeur canonique exportée (indépendante du round-trip time_t bogué). */
    cal_store_date(cal, y, m, d);

    /* Affichage : on positionne quand même le calendrier sur la date visée.
     * Un éventuel décalage d'un jour à l'affichage initial (dates hors saison
     * courante) est corrigé dès la première interaction ; la valeur exportée,
     * elle, reste juste. */
    struct tm t = {0};
    t.tm_year = y - 1900; t.tm_mon = m - 1; t.tm_mday = d; t.tm_isdst = -1;
    elm_calendar_selected_time_set(cal, &t);

    evas_object_smart_callback_add(cal, "changed", cal_changed_cb, NULL);
    evas_object_show(cal);
    return (GtkWidget *)cal;
}

gchar *widget_calendar_envvar_construct(GtkWidget *w)
{
    const char *stored = w ? evas_object_data_get((Evas_Object *)w, CAL_DATE_KEY) : NULL;
    if (stored) return g_strdup(stored);
    /* Repli : jamais atteint en pratique (create pose toujours la chaîne). */
    struct tm t = {0};
    elm_calendar_selected_time_get((Evas_Object *)w, &t);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", t.tm_year+1900, t.tm_mon+1, t.tm_mday);
    return g_strdup(buf);
}

gchar *widget_calendar_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_calendar_envvar_construct(v->Widget) : NULL; }

void widget_calendar_clear(variable *v) {
    if (!v || !v->Widget) return;
    time_t now = time(NULL); struct tm t; localtime_r(&now, &t);
    cal_store_date((Evas_Object *)v->Widget, t.tm_year+1900, t.tm_mon+1, t.tm_mday);
    elm_calendar_selected_time_set((Evas_Object *)v->Widget, &t);
}
void widget_calendar_refresh(variable *v) {}
void widget_calendar_fileselect(variable *v, const char *n, const char *val) {}
void widget_calendar_removeselected(variable *v) {}
void widget_calendar_save(variable *v) {}
