/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_calendar.cpp — Sélecteur de date FLTK (3 Fl_Spinner : J/M/A)
 * sermo — haplo-dialog — GPL-2.0-or-later
 * Export : "YYYY-MM-DD" */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_calendar.h"
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Spinner.H>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

struct CalGroup : Fl_Group {
    Fl_Spinner *day, *month, *year;
    CalGroup(int X,int Y,int W,int H) : Fl_Group(X,Y,W,H) {
        day  = new Fl_Spinner(X,      Y, 60, 30, "J");
        day->range(1,31); day->step(1);
        month = new Fl_Spinner(X+70,  Y, 60, 30, "M");
        month->range(1,12); month->step(1);
        year  = new Fl_Spinner(X+140, Y, 80, 30, "A");
        year->range(1900,2100); year->step(1);
        end();
        time_t t=time(NULL); struct tm *tm=localtime(&t);
        if (tm) { day->value(tm->tm_mday); month->value(tm->tm_mon+1); year->value(tm->tm_year+1900); }
    }
};

GtkWidget *widget_calendar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    CalGroup *cg = new CalGroup(0, 0, 240, 30);
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && strlen(def) >= 10) {
            int y=0,m=0,d=0;
            if (sscanf(def,"%d-%d-%d",&y,&m,&d)==3)
            { cg->year->value(y); cg->month->value(m); cg->day->value(d); }
        }
    }
    return (GtkWidget *)cg;
}
gchar *widget_calendar_envvar_construct(GtkWidget *w)
{
    CalGroup *cg = (CalGroup *)w;
    char buf[32];
    snprintf(buf,sizeof(buf),"%04d-%02d-%02d",(int)cg->year->value(),(int)cg->month->value(),(int)cg->day->value());
    return g_strdup(buf);
}
gchar *widget_calendar_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_calendar_envvar_construct(v->Widget) : NULL; }
void widget_calendar_clear(variable *v) {
    if (!v || !v->Widget) return;
    CalGroup *cg = (CalGroup *)v->Widget;
    time_t t=time(NULL); struct tm *tm=localtime(&t);
    if (tm) { cg->day->value(tm->tm_mday); cg->month->value(tm->tm_mon+1); cg->year->value(tm->tm_year+1900); }
}
void widget_calendar_refresh(variable *v) { if (v && v->Widget) ((Fl_Group *)v->Widget)->redraw(); }
void widget_calendar_fileselect(variable *v, const char*, const char*) {}
void widget_calendar_removeselected(variable *v) {}
void widget_calendar_save(variable *v) {}
