/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_spinner.c — Indicateur d'activité animé SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Affiche une animation de rotation (busy indicator).
 * render.cpp dessine un arc tournant via ImGui::DrawList.
 * La vitesse de rotation est configurable via l'attribut "speed" (degrés/frame).
 * L'animation peut être démarrée/arrêtée via actions :start: / :stop:.
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
#include "widget_spinner.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_spinner_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_SPINNER, NULL, "");
    node->state.spinner.angle = 0.0f;
    node->state.spinner.speed = 5.0;  /* 5°/frame = ~300°/s à 60fps */
    node->sensitive = FALSE; /* inactif par défaut */

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
    }
    return (GtkWidget *)node;
}

gchar *widget_spinner_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("false");
    return g_strdup(n->sensitive ? "active" : "inactive");
}
gchar *widget_spinner_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_spinner_envvar_construct(var->Widget);
}
void widget_spinner_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    n->state.spinner.angle = 0.0f;
    n->sensitive = FALSE;
}
void widget_spinner_refresh(variable *var) {}
void widget_spinner_fileselect(variable *var, const char *nm, const char *v) {}
void widget_spinner_removeselected(variable *var) {}
void widget_spinner_save(variable *var) {}
