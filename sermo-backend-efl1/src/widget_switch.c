/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_switch.c — Interrupteur booléen EFL (elm_check)
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
#include "widget_switch.h"
#include <Elementary.h>
#include <string.h>

GtkWidget *widget_switch_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *sw = elm_check_add(win);
    elm_object_style_set(sw, "toggle");
    elm_check_state_set(sw, EINA_FALSE);

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) elm_object_text_set(sw, lbl);
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (strcmp(def,"true")==0||strcmp(def,"1")==0||strcmp(def,"on")==0))
            elm_check_state_set(sw, EINA_TRUE);
    }
    evas_object_show(sw);
    return (GtkWidget *)sw;
}
gchar *widget_switch_envvar_construct(GtkWidget *w)
{ return g_strdup(elm_check_state_get((Evas_Object *)w) ? "true" : "false"); }
gchar *widget_switch_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_switch_envvar_construct(v->Widget) : NULL; }
void widget_switch_clear(variable *v)
{ if (v && v->Widget) elm_check_state_set((Evas_Object *)v->Widget, EINA_FALSE); }
void widget_switch_refresh(variable *v) {}
void widget_switch_fileselect(variable *v, const char *n, const char *val) {}
void widget_switch_removeselected(variable *v) {}
void widget_switch_save(variable *v) {}
