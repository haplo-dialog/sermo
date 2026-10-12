/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "lvgl-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_radiobutton.h"
#include <string.h>
#include <stdlib.h>

/* Le bouton coché du groupe ouvert. Le groupe est celui de l'étalon gtk3 : le
 * cœur le ferme (lastradiowidget = NULL) dès qu'un autre widget est créé. */
static WidgetNode *coche_du_groupe;

GtkWidget *widget_radiobutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_RADIOBUTTON, NULL, "");
    node->state.radio.selected = 0;
    /* Premier bouton d'un groupe : coché, comme GTK (un groupe radio a toujours
     * un choix). */
    if (lastradiowidget == NULL) {
        lastradiowidget = (GtkWidget *)node;
        coche_du_groupe = node;
        node->state.radio.selected = 1;
    }
    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (strcasecmp(def,"true")==0 || strcmp(def,"1")==0)) {
            if (coche_du_groupe && coche_du_groupe != node)
                coche_du_groupe->state.radio.selected = 0;
            coche_du_groupe = node;
            node->state.radio.selected = 1;
        }
    }
    return (GtkWidget *)node;
}

gchar *widget_radiobutton_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return g_strdup("false");
    return g_strdup(n->state.radio.selected ? "true" : "false");
}
gchar *widget_radiobutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_radiobutton_envvar_construct(var->Widget);
}
void widget_radiobutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.radio.selected = 0;
}
void widget_radiobutton_refresh(variable *var) {}
void widget_radiobutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_radiobutton_removeselected(variable *var) {}
void widget_radiobutton_save(variable *var) {}
