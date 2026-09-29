/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_statusbar.c — Barre de statut EFL (elm_label)
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
#include "widget_statusbar.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_statusbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *lbl = elm_label_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));

    if (Attr) {
        GList *el = NULL;
        gchar *txt = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (txt && *txt) elm_object_text_set(lbl, txt);
    }
    evas_object_size_hint_weight_set(lbl, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(lbl, EVAS_HINT_FILL, 0.5);
    evas_object_show(lbl);
    return (GtkWidget *)lbl;
}

gchar *widget_statusbar_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("");
    const char *txt = elm_object_text_get((Evas_Object *)w);
    return g_strdup(txt ? txt : "");
}
gchar *widget_statusbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_statusbar_envvar_construct(var->Widget);
}
void widget_statusbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_object_text_set((Evas_Object *)var->Widget, "");
}
void widget_statusbar_refresh(variable *var) {}
void widget_statusbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_statusbar_removeselected(variable *var) {}
void widget_statusbar_save(variable *var) {}
