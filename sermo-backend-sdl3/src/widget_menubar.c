/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubar.c — Widget menubar SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * SDL3_TODO: ImGui::BeginMenuBar disponible mais API différente.
 * Ce stub crée un noeud container pour les menuitems enfants.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_menubar.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_menubar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_MENUBAR, NULL, "menubar");

    /* Pop les menus enfants */
    stackelement s;
    int i;
    s = pop();
    for (i = 0; i < s.nwidgets; i++) {
        if (s.widgets[i])
            widget_node_add_child(node, (WidgetNode *)s.widgets[i]);
    }

    return (GtkWidget *)node;
}

gchar *widget_menubar_envvar_construct(GtkWidget *widget)
{
    return g_strdup("");
}

gchar *widget_menubar_envvar_all_construct(variable *var)
{
    return g_strdup("");
}

void widget_menubar_clear(variable *var) {}
void widget_menubar_refresh(variable *var) {}
void widget_menubar_fileselect(variable *var, const char *name, const char *value) {}
void widget_menubar_removeselected(variable *var) {}
void widget_menubar_save(variable *var) {}

/* <menu> : groupe ses <menuitem> en un sous-menu. Le rendu WT_MENUITEM avec
 * enfants ouvre un BeginMenu(label) ; on réutilise donc ce type pour le menu.
 * Le titre vient de l'attribut de balise label="…" (repli <label>). */
GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;
    const char *label = NULL;
    if (attr) { const char *tv = get_tag_attribute(attr, "label"); if (tv && *tv) label = tv; }
    if (!label && Attr) {
        GList *el = NULL;
        gchar *v = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (v && *v) label = v;
    }
    WidgetNode *node = widget_node_new(WT_MENUITEM, NULL, label ? label : "Menu");

    /* Dépiler les <menuitem> (SUM, ordre document) comme enfants. */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; i++)
        if (s.widgets[i])
            widget_node_add_child(node, (WidgetNode *)s.widgets[i]);

    return (GtkWidget *)node;
}
