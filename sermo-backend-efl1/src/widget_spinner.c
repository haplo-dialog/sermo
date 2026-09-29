/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_spinner.c — Indicateur de chargement EFL (elm_progressbar pulsé)
 * sermo — haplo-dialog — GPL-2.0-or-later */
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
#include "widget_spinner.h"
#include <Elementary.h>

GtkWidget *widget_spinner_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *pb = elm_progressbar_add(win);
    elm_progressbar_pulse_set(pb, EINA_TRUE);
    elm_progressbar_unit_format_set(pb, NULL);
    elm_progressbar_pulse(pb, EINA_TRUE);
    evas_object_show(pb);
    return (GtkWidget *)pb;
}
gchar *widget_spinner_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_spinner_envvar_all_construct(variable *v) { return NULL; }
void widget_spinner_clear(variable *v)
{ if (v && v->Widget) elm_progressbar_pulse((Evas_Object *)v->Widget, EINA_FALSE); }
void widget_spinner_refresh(variable *v)
{ if (v && v->Widget) elm_progressbar_pulse((Evas_Object *)v->Widget, EINA_TRUE); }
void widget_spinner_fileselect(variable *v, const char *n, const char *val) {}
void widget_spinner_removeselected(variable *v) {}
void widget_spinner_save(variable *v) {}
