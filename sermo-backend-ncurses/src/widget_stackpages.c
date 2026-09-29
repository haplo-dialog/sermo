/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_stackpages.c — N pages, une seule visible
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le nœud porte ses pages et l'index visible ; le renderer n'en dessine qu'une
 * (render_*.c, WT_STACK). Étalon = gtk3 (GtkStack).
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
#include "stack.h"
#include "widget_stackpages.h"
#include <stdlib.h>

GtkWidget *widget_stackpages_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;
    WidgetNode *node = widget_node_new(WT_STACK, NULL, "");
    int page = 0;

    node->state.notebook.side = 0;            /* 0 = pas de switcher */
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "page"))) page = atoi(v);
        if ((v = get_tag_attribute(attr, "switcher")) &&
            (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || atoi(v) == 1))
            node->state.notebook.side = 1;    /* rangée de boutons */
    }

    /* Un seul pop : le cœur coalesce les enfants (instruction SUM). */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *) s.widgets[i]);

    if (page < 0) page = 0;
    if (page >= node->child_count && node->child_count > 0) page = node->child_count - 1;
    node->state.notebook.current_tab = page;
    return (GtkWidget *) node;
}

/* Export : l'index de la page visible, comme <notebook> (étalon gtk3). */
gchar *widget_stackpages_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *) widget;
    char buf[16];
    if (!n) return g_strdup("0");
    snprintf(buf, sizeof(buf), "%d", n->state.notebook.current_tab);
    return g_strdup(buf);
}
gchar *widget_stackpages_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_stackpages_envvar_construct(var->Widget);
}
void widget_stackpages_clear(variable *var)          { (void) var; }
void widget_stackpages_refresh(variable *var)        { (void) var; }
void widget_stackpages_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_stackpages_removeselected(variable *var) { (void) var; }
void widget_stackpages_save(variable *var)           { (void) var; }
