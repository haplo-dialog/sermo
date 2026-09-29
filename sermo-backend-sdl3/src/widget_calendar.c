/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_calendar.c — Sélecteur de date SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Implémenté via 3 ImGui::InputInt (Jour / Mois / Année) + validation.
 * render.cpp affiche les 3 champs sur une ligne avec séparateurs "/".
 * La valeur exportée est "YYYY-MM-DD" (ISO 8601).
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_calendar.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

GtkWidget *widget_calendar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_CALENDAR, NULL, "Date");

    /* Date par défaut = aujourd'hui */
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    node->state.calendar.day   = tm ? tm->tm_mday : 1;
    node->state.calendar.month = tm ? tm->tm_mon + 1 : 1;
    node->state.calendar.year  = tm ? tm->tm_year + 1900 : 2026;

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && strlen(def) >= 10) {
            /* Format YYYY-MM-DD */
            int y = 0, m = 0, d = 0;
            if (sscanf(def, "%d-%d-%d", &y, &m, &d) == 3) {
                node->state.calendar.year  = y;
                node->state.calendar.month = m;
                node->state.calendar.day   = d;
            }
        }
    }
    return (GtkWidget *)node;
}

gchar *widget_calendar_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("1970-01-01");
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
             n->state.calendar.year,
             n->state.calendar.month,
             n->state.calendar.day);
    return g_strdup(buf);
}
gchar *widget_calendar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_calendar_envvar_construct(var->Widget);
}
void widget_calendar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    n->state.calendar.day = 1;
    n->state.calendar.month = 1;
    n->state.calendar.year = 2026;
}
void widget_calendar_refresh(variable *var) {}
void widget_calendar_fileselect(variable *var, const char *nm, const char *v) {}
void widget_calendar_removeselected(variable *var) {}
void widget_calendar_save(variable *var) {}
