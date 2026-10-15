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
#include "widget_checkbox.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *label = "";
    WidgetNode *node = widget_node_new(WT_CHECKBOX, NULL, label);
    node->state.checkbox.checked = FALSE;

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def) node->state.checkbox.checked =
            (strcasecmp(def,"true")==0 || strcmp(def,"1")==0);
    }
    /* Ses actions, jouées chaque fois qu'il bascule (clic ou « activate: ») :
     * voir jouer_actions_bascule(). Elles n'étaient pas gardées, donc jamais jouées. */
    if (Attr) {
        GList *ea = NULL;
        gchar *act = attributeset_get_first(&ea, Attr, ATTR_ACTION);
        if (act && *act) { node->action = strdup(act); node->actions_attr = (void *)Attr; }
    }
    return (GtkWidget *)node;
}

gchar *widget_checkbox_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return g_strdup("false");
    return g_strdup(n->state.checkbox.checked ? "true" : "false");
}
gchar *widget_checkbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_checkbox_envvar_construct(var->Widget);
}
void widget_checkbox_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.checkbox.checked = FALSE;
}
void widget_checkbox_refresh(variable *var) { widget_bascule_lire(var); }   /* widgets.c */
void widget_checkbox_fileselect(variable *var, const char *n, const char *v) {}
void widget_checkbox_removeselected(variable *var) {}
void widget_checkbox_save(variable *var) {}
