/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
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
#include "widget_radiobutton.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_radiobutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_RADIOBUTTON, NULL, "");
    node->state.radio.selected = 0;
    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (strcasecmp(def,"true")==0 || strcmp(def,"1")==0))
            node->state.radio.selected = 1;
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
