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
#include "widget_vbox.h"
#include <stdlib.h>

GtkWidget *widget_vbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_VBOX, NULL, "");
    /* Le coeur fusionne les enfants en UN element de pile (instruction
     * SUM) : UN SEUL pop, dans l'ordre du document. L'ancienne boucle
     * « pop jusqu'a une sentinelle » provoquait un stack underflow sur
     * TOUT dialogue (la sentinelle n'existe pas). */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *)s.widgets[i]);
    return (GtkWidget *)node;
}

gchar *widget_vbox_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_vbox_envvar_all_construct(variable *var) { return NULL; }
void   widget_vbox_clear(variable *var) {}
void   widget_vbox_refresh(variable *var) {}
void   widget_vbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_vbox_removeselected(variable *var) {}
void   widget_vbox_save(variable *var) {}
