/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_vseparator.c — Séparateur vertical EFL (elm_separator)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_vseparator.h"

GtkWidget *widget_vseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *sep = elm_separator_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    elm_separator_horizontal_set(sep, EINA_FALSE);
    evas_object_show(sep);
    return (GtkWidget *)sep;
}

gchar *widget_vseparator_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_vseparator_envvar_all_construct(variable *var) { return NULL; }
void   widget_vseparator_clear(variable *var) {}
void   widget_vseparator_refresh(variable *var) {}
void   widget_vseparator_fileselect(variable *var, const char *n, const char *v) {}
void   widget_vseparator_removeselected(variable *var) {}
void   widget_vseparator_save(variable *var) {}
