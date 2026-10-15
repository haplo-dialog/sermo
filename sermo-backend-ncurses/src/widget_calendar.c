/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_calendar.c — Sélecteur de date ncurses (terminal)
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
#include "ncurses-compat.h"
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
/* « AAAA-MM-JJ » → date choisie, règle de l'étalon (calendar_set_date_from_
 * string) : trois nombres lus par sscanf, mois borné à 1..12 et jour à 1..31,
 * sans contrôle de la longueur du mois (mesuré : 2021-02-31 reste tel quel).
 * Une chaîne illisible ne change rien. */
static void calendrier_poser(WidgetNode *n, const char *s)
{
    int y = 0, m = 0, d = 0;
    if (!s || sscanf(s, "%d-%d-%d", &y, &m, &d) != 3) return;
    n->state.calendar.year  = y;
    /* (la couche de compatibilité n'a pas CLAMP) */
    n->state.calendar.month = m < 1 ? 1 : m > 12 ? 12 : m;
    n->state.calendar.day   = d < 1 ? 1 : d > 31 ? 31 : d;
}

void widget_calendar_refresh(variable *var)
{
    /* <input> (commande ou fichier) : rejoué à chaque « refresh », comme
     * l'étalon gtk3sermo — sa PREMIÈRE ligne, CR/LF ôtés ; rien de lisible,
     * rien ne change ; aucune action ne joue. Jusqu'au 2026-10-13 il n'était
     * jamais lu. */
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    gchar *ligne = widget_input_ligne(var->Attributes);
    if (ligne) { calendrier_poser(n, ligne); g_free(ligne); }
    /* Premier passage : <default> l'emporte, l'étalon l'applique APRÈS
     * l'<input> (mesuré : <default> 2020-01-02 + <input> 2021-03-03 → 2020-01-02). */
    if (!n->initialised) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        n->initialised = TRUE;
        if (def) calendrier_poser(n, def);
    }
}
void widget_calendar_fileselect(variable *var, const char *nm, const char *v) {}
void widget_calendar_removeselected(variable *var) {}
void widget_calendar_save(variable *var) {}
