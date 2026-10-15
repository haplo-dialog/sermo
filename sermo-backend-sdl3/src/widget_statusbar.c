/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_statusbar.c — Widget statusbar SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
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

/* Le message vit dans node->label (render.cpp l'affiche, l'export le rend). */
static void statusbar_poser(WidgetNode *n, const char *texte)
{
    if (!texte) return;
    free(n->label);
    n->label = strdup(texte);
}

void widget_statusbar_refresh(variable *var)
{
    WidgetNode *n;
    gchar      *ligne;
    GList      *el = NULL;
    gchar      *v;

    if (!var || !var->Widget) return;
    /* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo (sa
     * première ligne = le message), sans jouer d'action. Au premier passage,
     * <label> puis <default> l'emportent, comme dans widget_statusbar_create() :
     * l'étalon les pousse après (mesuré). Jusqu'au 2026-10-13 il n'était
     * jamais lu. */
    n = (WidgetNode *)var->Widget;
    ligne = widget_input_ligne(var->Attributes);
    statusbar_poser(n, ligne);
    g_free(ligne);
    if (!n->initialised) {
        n->initialised = TRUE;
        v = attributeset_get_first(&el, var->Attributes, ATTR_LABEL);
        if (v && *v) statusbar_poser(n, v);
        el = NULL;
        v = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        if (v && *v) statusbar_poser(n, v);
    }
}
void widget_statusbar_fileselect(variable *var, const char *name, const char *value) {}
void widget_statusbar_removeselected(variable *var) {}
void widget_statusbar_save(variable *var) {}
