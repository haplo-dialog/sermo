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
#include "widget_window.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_window_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_WINDOW, NULL, "sdl3sermo");
    const char *tv;
    /* Titre : attribut de balise <window title="..."> d'abord, repli label */
    if (attr && (tv = get_tag_attribute(attr, "title")) && *tv) {
        free(node->label); node->label = strdup(tv);
    } else if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
    }
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "default-width")))  node->width  = atoi(v);
        if ((v = get_tag_attribute(attr, "default-height"))) node->height = atoi(v);
        if ((v = get_tag_attribute(attr, "width-request")))  node->width  = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) node->height = atoi(v);
    }
    stackelement s = pop();
    if (s.widgets[0]) widget_node_add_child(node, (WidgetNode *)s.widgets[0]);
    /* Le coeur ne passe plus la fenetre a gtk_main ; on la
     * capture ici pour que sermo_be_run_loop (= gtk_main) lance render_loop dessus. */
    extern WidgetNode *sdl3_root_window;
    sdl3_root_window = node;
    return (GtkWidget *)node;
}

gchar *widget_window_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_window_envvar_all_construct(variable *var) { return NULL; }
void   widget_window_clear(variable *var) {}
void   widget_window_refresh(variable *var) {}
void   widget_window_fileselect(variable *var, const char *n, const char *v) {}
void   widget_window_removeselected(variable *var) {}
void   widget_window_save(variable *var) {}
