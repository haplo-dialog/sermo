/* widget_calendar.cpp — Sélecteur de date Qt6 (QCalendarWidget)
 * qt6sermo 1.0.0 — haplo-dialog — GPL-2.0-or-later
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

GtkWidget *widget_calendar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QCalendarWidget *cal = new QCalendarWidget();
    cal->setGridVisible(true);
    cal->setNavigationBarVisible(true);
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && strlen(def) >= 10) {
            int y=0,m=0,d=0;
            if (sscanf(def, "%d-%d-%d",&y,&m,&d)==3)
                cal->setSelectedDate(QDate(y,m,d));
        }
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
void widget_calendar_refresh(variable *v)
{ if (v && v->Widget) static_cast<QCalendarWidget*>(v->Widget)->update(); }
void widget_calendar_fileselect(variable *v, const char*, const char*) {}
void widget_calendar_removeselected(variable *v) {}
void widget_calendar_save(variable *v) {}
