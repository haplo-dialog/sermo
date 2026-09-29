/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_linkbutton.c — Lien cliquable SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <linkbutton> : l'URI vient de <default>, le libellé de <label> (à défaut,
 * l'URI elle-même) — mêmes sources que l'étalon gtk3. Cliqué, il ouvre l'URI par
 * SDL_OpenURL (render.cpp) : la fonction de SDL3 qui passe la main au
 * navigateur du système. Pas de fork, pas de shell, aucune dépendance en
 * plus — l'URI n'est jamais interprétée par /bin/sh.
 *
 * Export : l'URI, comme l'étalon gtk3 (gtk_link_button_get_uri). Le stub
 * précédent exportait une chaîne VIDE — écart mesuré sur le banc.
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
#include "widget_linkbutton.h"
#include <stdlib.h>

GtkWidget *widget_linkbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) attr; (void) Type;
    const char *uri = NULL, *label = NULL;

    if (Attr) {
        GList *el = NULL;
        gchar *d = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (d) uri = d;
        el = NULL;
        gchar *l = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (l && *l) label = l;
    }
    if (!uri) uri = "";
    if (!label) label = uri;        /* étalon : sans <label>, l'URI fait office */

    WidgetNode *node = widget_node_new(WT_LINKBUTTON, NULL, label);
    snprintf(node->state.entry.buf, sizeof(node->state.entry.buf), "%s", uri);
    return (GtkWidget *) node;
}

gchar *widget_linkbutton_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *) widget;
    if (!n) return g_strdup("");
    return g_strdup(n->state.entry.buf);
}
gchar *widget_linkbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_linkbutton_envvar_construct(var->Widget);
}
void widget_linkbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *) var->Widget)->state.entry.buf[0] = '\0';
}
void widget_linkbutton_refresh(variable *var)        { (void) var; }
void widget_linkbutton_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_linkbutton_removeselected(variable *var) { (void) var; }
void widget_linkbutton_save(variable *var)           { (void) var; }
