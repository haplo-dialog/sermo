/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_revealer.c — Un enfant qui se montre et se cache
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * L'état vit dans state.toggle.active ; le renderer ne dessine l'enfant que
 * s'il est vrai. ⚠️ Pas de transition : ces ports montrent ou cachent. */
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
#include "widget_revealer.h"
#include <stdlib.h>

GtkWidget *widget_revealer_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;
    WidgetNode *node = widget_node_new(WT_REVEALER, NULL, "");
    int poses = 0;

    node->state.toggle.active = FALSE;
    if (attr) {
        const char *v = get_tag_attribute(attr, "reveal");
        if (v && (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || atoi(v) == 1))
            node->state.toggle.active = TRUE;
    }
    if (Attr && attributeset_is_avail(Attr, ATTR_DEFAULT)) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (!strcasecmp(def, "true") || !strcasecmp(def, "yes") || atoi(def) == 1))
            node->state.toggle.active = TRUE;
    }

    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i) {
        if (!s.widgets[i]) continue;
        if (poses == 0) widget_node_add_child(node, (WidgetNode *) s.widgets[i]);
        else fprintf(stderr, "sermo: <revealer> ne prend QU'UN enfant : le %de est "
                             "ignoré. Emballer le surplus dans une <vbox>.\n", poses + 1);
        poses++;
    }
    return (GtkWidget *) node;
}

/* Export : l'état courant, « true » ou « false » (étalon gtk3). */
gchar *widget_revealer_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *) widget;
    return g_strdup(n && n->state.toggle.active ? "true" : "false");
}
gchar *widget_revealer_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_revealer_envvar_construct(var->Widget);
}
void widget_revealer_clear(variable *var)          { if (var && var->Widget) ((WidgetNode *) var->Widget)->state.toggle.active = FALSE; }
void widget_revealer_refresh(variable *var)        { (void) var; }
void widget_revealer_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_revealer_removeselected(variable *var) { (void) var; }
void widget_revealer_save(variable *var)           { (void) var; }
