/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_infobar.c — Barre d'information colorée SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Affiche un message coloré selon le type :
 *   info     → bleu   (ImVec4(0.4, 0.7, 1.0, 1.0))
 *   warning  → orange (ImVec4(1.0, 0.7, 0.2, 1.0))
 *   error    → rouge  (ImVec4(1.0, 0.3, 0.3, 1.0))
 *   question → violet (ImVec4(0.8, 0.5, 1.0, 1.0))
 * render.cpp lit state.infobar.message_type pour choisir la couleur.
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
#include "widget_infobar.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_infobar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_INFOBAR, NULL, "");
    snprintf(node->state.infobar.message_type,
             sizeof(node->state.infobar.message_type), "info");
    node->state.infobar.revealed = TRUE;

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
    }
    return (GtkWidget *)node;
}

gchar *widget_infobar_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n || !n->label) return g_strdup("");
    return g_strdup(n->label);
}
gchar *widget_infobar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_infobar_envvar_construct(var->Widget);
}
void widget_infobar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    free(n->label); n->label = strdup("");
    n->state.infobar.revealed = FALSE;
}
void widget_infobar_refresh(variable *var) {}
void widget_infobar_fileselect(variable *var, const char *nm, const char *v) {}
void widget_infobar_removeselected(variable *var) {}
void widget_infobar_save(variable *var) {}
