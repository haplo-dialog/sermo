/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_checkbox.c — Case à cocher EFL/Elementary
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
#include "widget_checkbox.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *chk = elm_check_add(parent);

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) elm_object_text_set(chk, lbl);
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def) {
            int v = (strcasecmp(def,"true")==0 || strcmp(def,"1")==0);
            elm_check_state_set(chk, v);
        }
    }
    evas_object_show(chk);
    return (GtkWidget *)chk;
}

gchar *widget_checkbox_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("false");
    return g_strdup(elm_check_state_get((Evas_Object *)widget) ? "true" : "false");
}

gchar *widget_checkbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_checkbox_envvar_construct(var->Widget);
}

void widget_checkbox_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_check_state_set((Evas_Object *)var->Widget, 0);
}
void widget_checkbox_refresh(variable *var) {}
void widget_checkbox_fileselect(variable *var, const char *n, const char *v) {}
void widget_checkbox_removeselected(variable *var) {}
void widget_checkbox_save(variable *var) {}
