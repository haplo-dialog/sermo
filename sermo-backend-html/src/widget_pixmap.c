/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_pixmap.c — Widget pixmap du backend d'export HTML
 * sermo 2.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le chemin du fichier est stocké dans node->label, préfixé par le cœur :
 * « File:/chemin » pour un fichier, « File: » seul pour une icône de thème,
 * « Command:… » quand la valeur sort d'une commande. C'est render_html.c qui
 * lit ce champ et encode l'image en data: URI dans la page (html_emit_image).
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "html-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_pixmap.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_pixmap_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *path = "";
    if (Attr) {
        GList *el = NULL;
        gchar *v = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (v && *v) path = v;
        if (!*path) {
            el = NULL;
            v = attributeset_get_first(&el, Attr, ATTR_INPUT);
            if (v && *v) path = v;
        }
    }
    WidgetNode *node = widget_node_new(WT_PIXMAP, NULL, path);
    return (GtkWidget *)node;
}

gchar *widget_pixmap_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n || !n->label) return g_strdup("");
    return g_strdup(n->label);
}

gchar *widget_pixmap_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_pixmap_envvar_construct(var->Widget);
}

void widget_pixmap_clear(variable *var) {}
void widget_pixmap_refresh(variable *var) {}
void widget_pixmap_fileselect(variable *var, const char *name, const char *value) {}
void widget_pixmap_removeselected(variable *var) {}
void widget_pixmap_save(variable *var) {}
