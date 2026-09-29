/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_pixmap.c — Image EFL (elm_image)
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
#include "widget_pixmap.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_pixmap_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *img = elm_image_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));

    if (Attr) {
        GList *el = NULL;
        gchar *file = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (!file || !*file) {
            el = NULL;
            file = attributeset_get_first(&el, Attr, ATTR_INPUT);
        }
        if (file && *file) {
            elm_image_file_set(img, file, NULL);
            evas_object_data_set(img, "pixmap_file", strdup(file));
        }
    }
    evas_object_show(img);
    return (GtkWidget *)img;
}

gchar *widget_pixmap_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("");
    const char *f = (const char *)evas_object_data_get((Evas_Object *)w, "pixmap_file");
    return g_strdup(f ? f : "");
}
gchar *widget_pixmap_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_pixmap_envvar_construct(var->Widget);
}
void widget_pixmap_clear(variable *var) {}
void widget_pixmap_refresh(variable *var) {}
void widget_pixmap_fileselect(variable *var, const char *n, const char *v) {}
void widget_pixmap_removeselected(variable *var) {}
void widget_pixmap_save(variable *var) {}
