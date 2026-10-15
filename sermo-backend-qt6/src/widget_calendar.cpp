/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_calendar.cpp — Sélecteur de date Qt6 (QCalendarWidget)
 * sermo — haplo-dialog — GPL-2.0-or-later
 * Export : "YYYY-MM-DD" (ISO 8601) */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_calendar.h"
#include <QtWidgets/QCalendarWidget>
#include <QtCore/QDate>
#include <string.h>
#include <cstdio>

/* Date « AAAA-MM-JJ » lue comme l'étalon gtk3sermo (calendar_set_date_from_
 * string) : sscanf %d-%d-%d, mois borné à 1..12, jour à 1..31 (« 2021-13-40 »
 * donne 2021-12-31). Faux si le texte n'est pas une date : rien ne change.
 * Écart connu : QDate ne représente pas un jour au-delà de la fin du mois
 * (« 2021-02-31 », que GtkCalendar exporte tel quel) ; il est ramené au
 * dernier jour du mois (2021-02-28). */
static bool calendrier_lire(const char *texte, QDate *date)
{
    int y = 0, m = 0, d = 0;
    if (!texte || sscanf(texte, "%d-%d-%d", &y, &m, &d) != 3)
        return false;
    m = m < 1 ? 1 : (m > 12 ? 12 : m);
    d = d < 1 ? 1 : (d > 31 ? 31 : d);
    int fin = QDate(y, m, 1).daysInMonth();
    if (fin > 0 && d > fin)
        d = fin;
    *date = QDate(y, m, d);
    return date->isValid();
}

GtkWidget *widget_calendar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QCalendarWidget *cal = new QCalendarWidget();
    cal->setGridVisible(true);
    cal->setNavigationBarVisible(true);
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        QDate date;
        if (calendrier_lire(def, &date))
            cal->setSelectedDate(date);
    }
    return (GtkWidget *)cal;
}
gchar *widget_calendar_envvar_construct(GtkWidget *w)
{
    QDate d = static_cast<QCalendarWidget*>(w)->selectedDate();
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", d.year(), d.month(), d.day());
    return g_strdup(buf);
}
gchar *widget_calendar_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_calendar_envvar_construct(v->Widget) : NULL; }
void widget_calendar_clear(variable *v)
{ if (v && v->Widget) static_cast<QCalendarWidget*>(v->Widget)->setSelectedDate(QDate::currentDate()); }
/* <input> à chaque « refresh », comme l'étalon : la première ligne est une
 * date. Au premier passage (juste après la création), un <default> qui est une
 * date l'emporte — la commande tourne quand même. Aucune <action> n'est
 * branchée sur ce widget dans ce port : rien ne joue. Jusqu'au 2026-10-13,
 * l'<input> n'était jamais lu (CA restait à la date du jour). */
void widget_calendar_refresh(variable *v)
{
    if (!v || !v->Widget) return;
    QCalendarWidget *cal = static_cast<QCalendarWidget *>(v->Widget);
    bool premier = !cal->property("sermoLu").toBool();
    cal->setProperty("sermoLu", true);
    gchar *ligne = widget_input_line(v->Attributes);
    if (!ligne) return;
    QDate date;
    if (premier) {
        GList *el = NULL;
        if (calendrier_lire(attributeset_get_first(&el, v->Attributes, ATTR_DEFAULT), &date)) {
            g_free(ligne);
            return;
        }
    }
    if (calendrier_lire(ligne, &date))
        cal->setSelectedDate(date);
    g_free(ligne);
}
void widget_calendar_fileselect(variable *v, const char*, const char*) {}
void widget_calendar_removeselected(variable *v) {}
void widget_calendar_save(variable *v) {}
