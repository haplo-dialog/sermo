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
#include "widget_entry.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_entry_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_ENTRY, NULL, "");
    node->state.entry.buf[0] = '\0';

    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def)
            snprintf(node->state.entry.buf, sizeof(node->state.entry.buf), "%s", def);
        /* <input> : lu par widget_entry_refresh(), que le cœur appelle juste
         * après la création — le lire aussi ici exécuterait la commande deux
         * fois. */
    }
    return (GtkWidget *)node;
}

gchar *widget_entry_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return g_strdup("");
    return g_strdup(n->state.entry.buf);
}
gchar *widget_entry_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_entry_envvar_construct(var->Widget);
}
void widget_entry_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.entry.buf[0] = '\0';
}
void widget_entry_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    /* <input> (commande ou fichier) : la PREMIÈRE ligne, sans CR/LF, et elle
     * prime sur <default> — règle de l'étalon gtk3sermo (g_strchomp gardait
     * les lignes suivantes). */
    gchar *text = widget_input_text(var->Attributes);
    if (text) {
        text[strcspn(text, "\r\n")] = '\0';
        WidgetNode *node = (WidgetNode *)var->Widget;
        snprintf(node->state.entry.buf, sizeof(node->state.entry.buf), "%s", text);
        g_free(text);
    }
}
void widget_entry_fileselect(variable *var, const char *n, const char *v) {}
void widget_entry_removeselected(variable *var) {}
void widget_entry_save(variable *var) {}
