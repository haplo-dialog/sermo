/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_aspectframe.c — Cadre à rapport d'aspect EFL (elm_frame)
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
#include "widget_aspectframe.h"
#include <Elementary.h>
#include <string.h>

GtkWidget *widget_aspectframe_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *fr = elm_frame_add(win);
    elm_object_text_set(fr, "");
    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) elm_object_text_set(fr, lbl);
    }
    evas_object_show(fr);
    return (GtkWidget *)fr;
}
gchar *widget_aspectframe_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_aspectframe_envvar_all_construct(variable *v) { return NULL; }
void widget_aspectframe_clear(variable *v) {}
void widget_aspectframe_refresh(variable *v) {}
void widget_aspectframe_fileselect(variable *v, const char *n, const char *val) {}
void widget_aspectframe_removeselected(variable *v) {}
void widget_aspectframe_save(variable *v) {}
