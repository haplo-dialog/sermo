/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menuitem.c — Widget menuitem ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "ncurses-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_menuitem.h"
#include "sermo_icon_theme.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_menuitem_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *label = "";
    if (Attr) {
        GList *el = NULL;
        gchar *v = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (v && *v) label = v;
    }
    WidgetNode *node = widget_node_new(WT_MENUITEM, NULL, label);

    /* menuitem cochable : checkbox="true|false" / radiobutton="true|false"
     * — l'etat par defaut est exporte en variable (banc 04). */
    if (attr) {
        const char *v = get_tag_attribute(attr, "checkbox");
        if (!v) v = get_tag_attribute(attr, "radiobutton");
        if (v)
            node->state.checkbox.checked =
                (strcasecmp(v, "true") == 0 || strcmp(v, "1") == 0);
    }
    if (Attr) {
        GList *el = NULL;
        gchar *act = attributeset_get_first(&el, Attr, ATTR_ACTION);
        if (act && *act)
            node->action = strdup(act);
    }

    /* Icône de thème (parité gtk3) : stock → icon/icon-name/image-icon →
     * image-name/image-file ; 16 px par défaut (theme-icon-size surcharge). */
    if (attr) {
        const char *icon = get_tag_attribute(attr, "icon");
        if (!icon) icon = get_tag_attribute(attr, "icon-name");
        if (!icon) icon = get_tag_attribute(attr, "image-icon");
        if (!icon) icon = get_tag_attribute(attr, "stock");
        if (!icon) icon = get_tag_attribute(attr, "stock-id");
        const char *file = get_tag_attribute(attr, "image-name");
        if (!file) file = get_tag_attribute(attr, "image-file");
        int px = sermo_icon_size_px("menu");   /* 16 */
        const char *tv = get_tag_attribute(attr, "theme-icon-size");
        if (tv && atoi(tv) > 0) px = atoi(tv);
        if (icon && *icon)      { node->icon_path = sermo_icon_lookup(icon, px); node->icon_px = px; }
        else if (file && *file) { node->icon_path = strdup(file); node->icon_px = px; }
    }

    return (GtkWidget *)node;
}

gchar *widget_menuitem_envvar_construct(GtkWidget *widget)
{
    /* menuitem cochable : exporte son etat (les menuitems simples n'ont
     * en pratique pas de <variable>, donc ne passent pas par ici) */
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("");
    return g_strdup(n->state.checkbox.checked ? "true" : "false");
}

gchar *widget_menuitem_envvar_all_construct(variable *var)
{
    return g_strdup("");
}

void widget_menuitem_clear(variable *var) {}
void widget_menuitem_refresh(variable *var) {}
void widget_menuitem_fileselect(variable *var, const char *name, const char *value) {}
void widget_menuitem_removeselected(variable *var) {}
void widget_menuitem_save(variable *var) {}
