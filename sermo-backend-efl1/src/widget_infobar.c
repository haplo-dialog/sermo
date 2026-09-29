/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_infobar.c — Barre d'information EFL (elm_notify + elm_label)
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
#include "widget_infobar.h"
#include <Elementary.h>
#include <string.h>

GtkWidget *widget_infobar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *box = elm_box_add(win);
    elm_box_horizontal_set(box, EINA_TRUE);
    Evas_Object *lbl = elm_label_add(box);
    elm_label_line_wrap_set(lbl, ELM_WRAP_WORD);

    if (Attr) {
        GList *el = NULL;
        gchar *msg = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (!msg || !*msg) msg = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (msg && *msg) {
            char markup[1024];
            snprintf(markup, sizeof(markup), "<info>ℹ</info> %s", msg);
            elm_object_text_set(lbl, markup);
            /* texte BRUT pour l'export (parite : IB="attention") */
            evas_object_data_set(box, "ib_text", strdup(msg));
        }
    }
    elm_box_pack_end(box, lbl);
    evas_object_show(lbl);
    evas_object_show(box);
    return (GtkWidget *)box;
}
gchar *widget_infobar_envvar_construct(GtkWidget *w)
{
    const char *txt;
    if (!w) return g_strdup("");
    txt = (const char *)evas_object_data_get((Evas_Object *)w, "ib_text");
    return g_strdup(txt ? txt : "");
}
gchar *widget_infobar_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_infobar_envvar_construct(v->Widget) : NULL; }
void widget_infobar_clear(variable *v) {}
void widget_infobar_refresh(variable *v) {}
void widget_infobar_fileselect(variable *v, const char *n, const char *val) {}
void widget_infobar_removeselected(variable *v) {}
void widget_infobar_save(variable *v) {}
