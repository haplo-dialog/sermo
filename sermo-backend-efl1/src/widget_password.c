/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_password.c — Champ mot de passe EFL (elm_entry password mode)
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
#include "widget_password.h"
#include <Elementary.h>
#include <string.h>

GtkWidget *widget_password_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *en = elm_entry_add(win);
    elm_entry_single_line_set(en, EINA_TRUE);
    elm_entry_password_set(en, EINA_TRUE);
    elm_entry_scrollable_set(en, EINA_TRUE);   /* cadre visible, comme l'entry */
    evas_object_size_hint_min_set(en, 120, 27);
    elm_object_text_set(en, "");
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) elm_entry_entry_set(en, def);
    }
    evas_object_show(en);
    return (GtkWidget *)en;
}
gchar *widget_password_envvar_construct(GtkWidget *w)
{ const char *t = elm_entry_entry_get((Evas_Object *)w); return g_strdup(t ? t : ""); }
gchar *widget_password_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_password_envvar_construct(v->Widget) : NULL; }
void widget_password_clear(variable *v)
{ if (v && v->Widget) elm_entry_entry_set((Evas_Object *)v->Widget, ""); }
void widget_password_refresh(variable *v) {}
void widget_password_fileselect(variable *v, const char *n, const char *val) {}
void widget_password_removeselected(variable *v) {}
void widget_password_save(variable *v) {}
