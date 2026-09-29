/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_password.c — Champ mot de passe ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Utilise ImGui::InputText avec ImGuiInputTextFlags_Password.
 * La valeur masquée est exportée en clair dans la variable d'environnement.
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
#include "widget_password.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_password_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_PASSWORD, NULL, "");
    node->state.entry.buf[0] = '\0';

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def)
            snprintf(node->state.entry.buf, sizeof(node->state.entry.buf), "%s", def);
    }
    return (GtkWidget *)node;
}

gchar *widget_password_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("");
    return g_strdup(n->state.entry.buf);
}
gchar *widget_password_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_password_envvar_construct(var->Widget);
}
void widget_password_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.entry.buf[0] = '\0';
}
void widget_password_refresh(variable *var) {}
void widget_password_fileselect(variable *var, const char *n, const char *v) {}
void widget_password_removeselected(variable *var) {}
void widget_password_save(variable *var) {}
