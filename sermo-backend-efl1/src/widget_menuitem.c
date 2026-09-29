/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_menuitem.c — <menuitem> EFL : carrier de modèle poussé sur la pile,
 * consommé par <menu>/<menubar>. GPL-2.0-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "automaton.h"
#include "attributes.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_menuitem.h"
#include "efl_menu_model.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_menuitem_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    EMenuItem *mi = calloc(1, sizeof(EMenuItem));
    if (Type == WIDGET_MENUITEMSEPARATOR) mi->separator = 1;

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) mi->label = strdup(lbl);
        el = NULL;
        gchar *cmd = attributeset_get_first(&el, Attr, ATTR_ACTION);
        if (cmd && *cmd) mi->cmd = strdup(cmd);
    }
    if (!mi->label && attr) {
        const char *tv = get_tag_attribute(attr, "label");
        if (tv && *tv) mi->label = strdup(tv);
    }
    if (attr) {
        const char *icon = get_tag_attribute(attr, "icon");
        if (!icon) icon = get_tag_attribute(attr, "icon-name");
        if (!icon) icon = get_tag_attribute(attr, "image-icon");
        if (!icon) icon = get_tag_attribute(attr, "stock");
        if (!icon) icon = get_tag_attribute(attr, "stock-id");
        if (icon && *icon) mi->icon = strdup(icon);
        const char *v = get_tag_attribute(attr, "checkbox");
        if (!v) v = get_tag_attribute(attr, "radiobutton");
        if (v) { mi->has_check = 1;
                 mi->check_val = (strcasecmp(v, "true") == 0 || strcmp(v, "1") == 0); }
    }

    Evas_Object *win = efl_main_win_get();
    Evas_Object *carrier = evas_object_rectangle_add(evas_object_evas_get(win));
    evas_object_data_set(carrier, "emi", mi);
    return (GtkWidget *)carrier;
}

gchar *widget_menuitem_envvar_construct(GtkWidget *w)
{
    if (!w) return NULL;
    EMenuItem *mi = (EMenuItem *)evas_object_data_get((Evas_Object *)w, "emi");
    if (!mi || !mi->has_check) return NULL;
    return g_strdup(mi->check_val ? "true" : "false");
}
gchar *widget_menuitem_envvar_all_construct(variable *var)
{ return var && var->Widget ? widget_menuitem_envvar_construct(var->Widget) : NULL; }
void   widget_menuitem_clear(variable *var) {}
void   widget_menuitem_refresh(variable *var) {}
void   widget_menuitem_fileselect(variable *var, const char *n, const char *v) {}
void   widget_menuitem_removeselected(variable *var) {}
void   widget_menuitem_save(variable *var) {}
