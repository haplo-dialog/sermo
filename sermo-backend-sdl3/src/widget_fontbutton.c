/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_fontbutton.c — Widget fontbutton SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * SDL3_TODO: le sélecteur de police sera câblé dans render.cpp.
 * Ce stub stocke la police courante dans node->label.
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
#include "widget_fontbutton.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_fontbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *font = "Sans 12";
    if (Attr) {
        GList *el = NULL;
        gchar *v = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (v && *v) font = v;
    }
    WidgetNode *node = widget_node_new(WT_BUTTON, NULL, font);
    if (node->var_name) free(node->var_name);
    node->var_name = strdup("__fontbutton__");
    return (GtkWidget *)node;
}

gchar *widget_fontbutton_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n || !n->label) return g_strdup("Sans 12");
    return g_strdup(n->label);
}

gchar *widget_fontbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_fontbutton_envvar_construct(var->Widget);
}

void widget_fontbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    free(n->label);
    n->label = strdup("Sans 12");
}

/* Le nom de police vit dans node->label (le bouton l'affiche tel quel). */
static void fontbutton_poser(WidgetNode *n, const char *police)
{
    if (!police) return;
    free(n->label);
    n->label = strdup(police);
}

void widget_fontbutton_refresh(variable *var)
{
    WidgetNode *n;
    gchar      *ligne;
    GList      *el = NULL;

    if (!var || !var->Widget) return;
    /* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo (sa
     * première ligne = le nom de police, « Sans 13 »), sans jouer d'action.
     * Au premier passage, un <default> l'emporte : l'étalon l'applique après
     * (mesuré). Jusqu'au 2026-10-13 il n'était jamais lu. */
    n = (WidgetNode *)var->Widget;
    ligne = widget_input_ligne(var->Attributes);
    fontbutton_poser(n, ligne);
    g_free(ligne);
    if (!n->initialised) {
        n->initialised = TRUE;
        fontbutton_poser(n, attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT));
    }
}
void widget_fontbutton_fileselect(variable *var, const char *name, const char *value) {}
void widget_fontbutton_removeselected(variable *var) {}
void widget_fontbutton_save(variable *var) {}
