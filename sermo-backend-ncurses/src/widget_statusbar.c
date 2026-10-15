/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_statusbar.c — Widget statusbar ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_statusbar.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_statusbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *label = "";
    if (Attr) {
        GList *el = NULL;
        gchar *v = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (v && *v) label = v;
        /* gtk3sermo pousse <label> puis <default> : le texte montré (et la variable)
         * est <default> s'il est là. Ignoré jusqu'à la 2.8.0 : barre vide. */
        el = NULL;
        v = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (v && *v) label = v;
    }
    WidgetNode *node = widget_node_new(WT_STATUSBAR, NULL, label);
    return (GtkWidget *)node;
}

gchar *widget_statusbar_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n || !n->label) return g_strdup("");
    return g_strdup(n->label);
}

gchar *widget_statusbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_statusbar_envvar_construct(var->Widget);
}

void widget_statusbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    free(n->label);
    n->label = strdup("");
}

void widget_statusbar_refresh(variable *var)
{
    /* <input> (commande ou fichier) : rejoué à chaque « refresh », comme
     * l'étalon gtk3sermo — sa PREMIÈRE ligne, CR/LF ôtés ; rien de lisible,
     * rien ne change ; aucune action ne joue. Jusqu'au 2026-10-13 il n'était
     * jamais lu. */
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    gchar *ligne = widget_input_ligne(var->Attributes);
    if (ligne) { free(n->label); n->label = strdup(ligne); g_free(ligne); }
    /* Premier passage : <label> puis <default> l'emportent, l'étalon les
     * pousse APRÈS l'<input> (mesuré : <label> L + <input> « in » → L). */
    if (!n->initialised) {
        GList *el = NULL;
        gchar *v = attributeset_get_first(&el, var->Attributes, ATTR_LABEL);
        n->initialised = TRUE;
        if (v && *v) { free(n->label); n->label = strdup(v); }
        el = NULL;
        v = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        if (v && *v) { free(n->label); n->label = strdup(v); }
    }
}
void widget_statusbar_fileselect(variable *var, const char *name, const char *value) {}
void widget_statusbar_removeselected(variable *var) {}
void widget_statusbar_save(variable *var) {}
