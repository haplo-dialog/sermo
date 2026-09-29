/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_searchentry.c — Champ de recherche EFL (elm_entry + icon)
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
#include "widget_searchentry.h"
#include <Elementary.h>
#include <string.h>

GtkWidget *widget_searchentry_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *en = elm_entry_add(win);
    elm_entry_single_line_set(en, EINA_TRUE);
    elm_object_part_text_set(en, "guide", "Rechercher...");
    Evas_Object *ic = elm_icon_add(en);
    elm_icon_standard_set(ic, "edit-find");
    elm_object_part_content_set(en, "icon", ic);
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) elm_entry_entry_set(en, def);
    }
    evas_object_show(en);
    return (GtkWidget *)en;
}
gchar *widget_searchentry_envvar_construct(GtkWidget *w)
{ const char *t = elm_entry_entry_get((Evas_Object *)w); return g_strdup(t ? t : ""); }
gchar *widget_searchentry_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_searchentry_envvar_construct(v->Widget) : NULL; }
void widget_searchentry_clear(variable *v)
{ if (v && v->Widget) elm_entry_entry_set((Evas_Object *)v->Widget, ""); }
void widget_searchentry_refresh(variable *v) {}
void widget_searchentry_fileselect(variable *v, const char *n, const char *val) {}
void widget_searchentry_removeselected(variable *v) {}
void widget_searchentry_save(variable *v) {}
