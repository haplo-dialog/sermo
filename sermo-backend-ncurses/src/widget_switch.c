/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_switch.c — Interrupteur booléen ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ImGui n'a pas de widget Toggle natif. On simule avec un Checkbox
 * stylisé : label "[ ON ]" / "[ OFF ]" selon l'état.
 * render.cpp affichera un bouton coloré selon l'état.
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
#include "widget_switch.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_switch_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_SWITCH, NULL, "Switch");
    node->state.toggle.active = FALSE;

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def) {
            if (strcmp(def,"true")==0 || strcmp(def,"1")==0 || strcmp(def,"on")==0)
                node->state.toggle.active = TRUE;
        }
    }
    return (GtkWidget *)node;
}

gchar *widget_switch_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("false");
    return g_strdup(n->state.toggle.active ? "true" : "false");
}
gchar *widget_switch_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_switch_envvar_construct(var->Widget);
}
void widget_switch_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.toggle.active = FALSE;
}
void widget_switch_refresh(variable *var) {}
void widget_switch_fileselect(variable *var, const char *n, const char *v) {}
void widget_switch_removeselected(variable *var) {}
void widget_switch_save(variable *var) {}
