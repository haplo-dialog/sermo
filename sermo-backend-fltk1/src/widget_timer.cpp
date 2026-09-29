/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_timer.cpp — Minuterie FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <timer> → widget invisible (Fl_Box de taille nulle) + Fl::add_timeout()
 *
 * Attributs :
 *   interval   : délai en secondes (float, défaut 1.0)
 *   milliseconds : délai en ms (alternative à interval)
 *   visible    : si "false", widget invisible (défaut)
 *   function   : action à exécuter à chaque tick (via safe_exec)
 *
 * Export : nombre de ticks écoulés depuis la création
 *
 * Note : le timer démarre automatiquement à la création.
 *        Il est répétitif (auto-rechargé dans le callback).
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
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

struct TimerData {
    double        interval;   /* secondes */
    AttributeSet *Attr;       /* actions itérées à chaque tick */
    long          ticks;      /* compteur de ticks */
    gboolean      enabled;    /* exporté "true"/"false" (parité étalon) */
    Fl_Box       *widget;     /* référence pour redraw éventuel */
};

static void timer_cb(void *data)
{
    TimerData *td = (TimerData *)data;
    td->ticks++;

    /* TOUTES les actions, via le répartiteur du cœur (exit:, refresh:,
     * commande shell…) — safe_system direct traitait « exit:fin » comme
     * une commande et le dialogue ne se fermait jamais. */
    if (td->Attr) {
        GList *element = NULL;
        gchar *fn = attributeset_get_first(&element, td->Attr, ATTR_ACTION);
        while (fn) {
            if (*fn) execute_action((GtkWidget *)td->widget, fn, NULL);
            fn = attributeset_get_next(&element, td->Attr, ATTR_ACTION);
        }
    }

    /* Replanifier */
    Fl::add_timeout(td->interval, timer_cb, data);
}

/* ⛔ atof suit la LOCALE : sous fr_FR, atof("0.5") rend ZÉRO en silence.
 * g_ascii_strtod lit toujours le point décimal. La garde
 * tests/garde_fonctions_interdites.sh l'exige — elle ne regardait pas les
 * .cpp jusqu'au 2026-09-14, d'où ces appels restés en place. */
GtkWidget *widget_timer_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    /* Widget invisible de taille 0 (placeholder pour la hiérarchie) */
    Fl_Box *box = new Fl_Box(0, 0, 0, 0, nullptr);
    box->hide();

    TimerData *td = new TimerData;
    td->interval = 1.0;
    td->Attr     = Attr;
    td->ticks    = 0;
    td->enabled  = TRUE;
    td->widget   = box;

    if (attr) {
        const char *v;
        /* g_ascii_strtod, pas atof : sous locale fr_FR, g_ascii_strtod("0.5", NULL) rend 0
         * en silence (piège documenté). */
        if ((v = get_tag_attribute(attr, "interval")))
            td->interval = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "milliseconds")))
            td->interval = g_ascii_strtod(v, NULL) / 1000.0;
        if (td->interval <= 0) td->interval = 1.0;
    }

    box->user_data(td);

    /* Démarrer le timer */
    Fl::add_timeout(td->interval, timer_cb, td);

    return (GtkWidget *)box;
}

gchar *widget_timer_envvar_construct(GtkWidget *widget)
{
    /* Parité de VALEUR avec l'étalon : un timer exporte son état
     * "true"/"false", pas un compteur de ticks. */
    Fl_Box *box = (Fl_Box *)widget;
    if (!box) return g_strdup("false");
    TimerData *td = (TimerData *)box->user_data();
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
    Fl_Box *box = (Fl_Box *)var->Widget;
    TimerData *td = (TimerData *)box->user_data();
    if (td) {
        Fl::remove_timeout(timer_cb, td);
        td->ticks = 0;
        td->enabled = FALSE;
    }
}

void widget_timer_refresh(variable *var) {}

void widget_timer_fileselect(variable *var, const char *n, const char *v) {}
void widget_timer_removeselected(variable *var) {}
void widget_timer_save(variable *var) {}
