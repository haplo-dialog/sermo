/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
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
#include "stack.h"
#include "widget_frame.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_frame_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_FRAME, NULL, "");
    if (attr) {
        const char *tv = get_tag_attribute(attr, "label");
        if (tv && *tv) { free(node->label); node->label = strdup(tv); }
    }
    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
    }
    /* TOUS les enfants, comme l'étalon, qui les range dans une boîte
     * verticale (sermo-backend-gtk3/src/widget_frame.c). Jusqu'à la 2.6.8 ce
     * port ne gardait que le premier : les suivants étaient créés mais
     * jamais dessinés — et une <progressbar> en deuxième position, jamais
     * relevée, ne déclenchait plus son action. */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; i++)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *)s.widgets[i]);
    return (GtkWidget *)node;
}

/* L'étalon exporte le TITRE du cadre (gtk_frame_get_label) ; un cadre sans
 * titre rend une chaîne vide. Ce port rendait TOUJOURS vide — le cas 38 du banc
 * de comportement l'a mesuré. Un conteneur qui n'exporte rien, ça se décrète ;
 * ici l'étalon exporte, donc on exporte. */
gchar *widget_frame_envvar_construct(GtkWidget *w)
{
    WidgetNode *node = (WidgetNode *)w;
    return g_strdup(node && node->label ? node->label : "");
}
gchar *widget_frame_envvar_all_construct(variable *var) { return NULL; }
void   widget_frame_clear(variable *var) {}
void   widget_frame_refresh(variable *var) {}
void   widget_frame_fileselect(variable *var, const char *n, const char *v) {}
void   widget_frame_removeselected(variable *var) {}
void   widget_frame_save(variable *var) {}
