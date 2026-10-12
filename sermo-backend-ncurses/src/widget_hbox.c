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
#include "stack.h"
#include "widget_hbox.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>

extern gint project_space_expand, project_space_fill;   /* coeur (gtkdialog.c) */

static int vrai(const char *v)
{
    return strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 || atoi(v) == 1;
}

GtkWidget *widget_hbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_HBOX, NULL, "");
    /* Le coeur fusionne les enfants en UN element de pile (instruction
     * SUM) : UN SEUL pop, dans l'ordre du document. L'ancienne boucle
     * « pop jusqu'a une sentinelle » provoquait un stack underflow sur
     * TOUT dialogue (la sentinelle n'existe pas). */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *)s.widgets[i]);

    /* Étirement : la règle de l'étalon gtk3 (sermo-backend-gtk3/src/widget_hbox.c).
     * Valeur courante = celle de la boîte, sinon « non dit » ; un enfant qui
     * dit la sienne la REMPLACE pour lui et pour les suivants (RANGEE : du dernier au premier, comme gtk_box_pack_end) ;
     * « non dit » = le défaut du type. */
    int ex = project_space_expand, fi = project_space_fill;
    const char *v;
    if (attr && (v = get_tag_attribute(attr, "space-expand"))) ex = vrai(v);
    if (attr && (v = get_tag_attribute(attr, "space-fill")))   fi = vrai(v);
    if (attr && (v = get_tag_attribute(attr, "homogeneous")))  node->homogeneous = vrai(v);
    for (int i = s.nwidgets - 1; i >= 0; --i) {
        WidgetNode *c = (WidgetNode *)s.widgets[i];
        if (!c) continue;
        if (c->expand) ex = c->expand > 0;
        if (c->fill)   fi = c->fill > 0;
        int t = s.widgettypes[i];
        int d = (t == WIDGET_EDIT || t == WIDGET_FRAME || t == WIDGET_SCROLLEDW ||
                 t == WIDGET_ENTRY);
        c->pack_expand = (signed char)(ex != -1 ? ex : d);
        c->pack_fill   = (signed char)(fi != -1 ? fi : d);
    }
    return (GtkWidget *)node;
}

gchar *widget_hbox_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_hbox_envvar_all_construct(variable *var) { return NULL; }
void   widget_hbox_clear(variable *var) {}
void   widget_hbox_refresh(variable *var) {}
void   widget_hbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_hbox_removeselected(variable *var) {}
void   widget_hbox_save(variable *var) {}
