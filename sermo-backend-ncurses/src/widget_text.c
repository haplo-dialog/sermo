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
#include "widget_text.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_text_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_TEXT, NULL, "");
    node->state.text.content = strdup("");
    node->state.text.len = 0;

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) { free(node->state.text.content);
            node->state.text.content = strdup(def); }
        /* <input> : lu par widget_text_refresh(), que le cœur appelle juste
         * après la création. */
    }
    node->state.text.len = (int)strlen(node->state.text.content);
    return (GtkWidget *)node;
}

gchar *widget_text_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return g_strdup("");
    return g_strdup(n->state.text.content ? n->state.text.content : "");
}
gchar *widget_text_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_text_envvar_construct(var->Widget);
}
void widget_text_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    free(n->state.text.content); n->state.text.content = strdup(""); n->state.text.len = 0;
}
void widget_text_refresh(variable *var)
{
    if (!var || !var->Widget || !var->Attributes) return;
    /* <input> (commande ou fichier) : le contenu ENTIER, saut de ligne final
     * compris — règle de l'étalon gtk3sermo. Jusqu'à la 2.7.0 la directive
     * brute (« Command:… », « file:… ») partait telle quelle vers le shell :
     * rien n'était lu. */
    gchar *text = widget_input_text(var->Attributes);
    if (text) {
        WidgetNode *n = (WidgetNode *)var->Widget;
        free(n->state.text.content);
        n->state.text.content = strdup(text);
        n->state.text.len = (int)strlen(n->state.text.content);
        g_free(text);
    }
}
void widget_text_fileselect(variable *var, const char *n, const char *v) {}
void widget_text_removeselected(variable *var) {}
void widget_text_save(variable *var) {}
