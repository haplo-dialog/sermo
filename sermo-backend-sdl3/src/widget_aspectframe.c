/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_aspectframe.c — Frame avec ratio d'aspect imposé SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ImGui n'a pas de widget aspect-ratio natif.
 * On simule via ImGui::BeginChild avec une taille calculée :
 *   width = available_width
 *   height = width / ratio
 * render.cpp lit state.aspect.ratio et calcule la hauteur en conséquence.
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
#include "widget_aspectframe.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_aspectframe_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_ASPECTFRAME, NULL, "");
    node->state.aspect.ratio = 1.0f;  /* carré par défaut */

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
    }

    /* Attacher l'enfant depuis la pile */
    stackelement s;
    extern stackelement pop(void);
    s = pop();
    for (int i = 0; i < s.nwidgets; i++)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *)s.widgets[i]);

    return (GtkWidget *)node;
}

gchar *widget_aspectframe_envvar_construct(GtkWidget *widget)
{
    return g_strdup("");
}
gchar *widget_aspectframe_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_aspectframe_envvar_construct(var->Widget);
}
void widget_aspectframe_clear(variable *var) {}
void widget_aspectframe_refresh(variable *var) {}
void widget_aspectframe_fileselect(variable *var, const char *nm, const char *v) {}
void widget_aspectframe_removeselected(variable *var) {}
void widget_aspectframe_save(variable *var) {}
