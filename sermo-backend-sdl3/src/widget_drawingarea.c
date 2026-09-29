/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_drawingarea.c — Zone de dessin libre SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Utilise ImGui::GetWindowDrawList() pour permettre le dessin vectoriel.
 * Le script shell peut injecter des commandes de dessin via l'action.
 * Format des commandes (via pipe) :
 *   line x1 y1 x2 y2 #rrggbb thick
 *   rect x1 y1 x2 y2 #rrggbb filled
 *   circle cx cy r #rrggbb filled
 *   text x y "message" #rrggbb
 * render.cpp lit state.drawing et exécute les commandes ImDrawList.
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
#include "widget_drawingarea.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_drawingarea_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_DRAWINGAREA, NULL, "canvas");
    node->state.drawing.width    = 300.0f;
    node->state.drawing.height   = 200.0f;
    node->state.drawing.bg_color[0] = '\0';

    if (Attr) {
        GList *el = NULL;
        gchar *w = attributeset_get_first(&el, Attr, ATTR_WIDTH);
        if (w) node->state.drawing.width  = (float)g_ascii_strtod(w, NULL);
        el = NULL;
        gchar *h = attributeset_get_first(&el, Attr, ATTR_HEIGHT);
        if (h) node->state.drawing.height = (float)g_ascii_strtod(h, NULL);
        el = NULL;
        gchar *act = attributeset_get_first(&el, Attr, ATTR_ACTION);
        if (act && *act) node->action = strdup(act);
    }

    node->width  = (int)node->state.drawing.width;
    node->height = (int)node->state.drawing.height;

    return (GtkWidget *)node;
}

gchar *widget_drawingarea_envvar_construct(GtkWidget *widget)
{
    /* Étalon gtk3 : une zone de dessin n'exporte AUCUNE valeur. Ce port
     * rendait « canvas », ce qui n'a jamais été vu parce que le répartiteur
     * d'export ne l'appelait pas (corrigé le 2026-09-14). */
    (void) widget;
    return g_strdup("");
}
gchar *widget_drawingarea_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_drawingarea_envvar_construct(var->Widget);
}
void widget_drawingarea_clear(variable *var) {}
void widget_drawingarea_refresh(variable *var) {}
void widget_drawingarea_fileselect(variable *var, const char *nm, const char *v) {}
void widget_drawingarea_removeselected(variable *var) {}
void widget_drawingarea_save(variable *var) {}
