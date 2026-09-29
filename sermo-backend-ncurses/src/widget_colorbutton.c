/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_colorbutton.c — Sélecteur de couleur ncurses (terminal) v2
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * v2 : utilise WT_COLORBUTTON + state.color (ColorState).
 * render.cpp appelle ImGui::ColorEdit3 ou ImGui::ColorButton + popup.
 * L'export est "#rrggbb" (hex 6 digits).
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
#include "widget_colorbutton.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Conversion #rrggbb → float RGB */
static void parse_hex_color(const char *hex, ColorState *c)
{
    c->r = c->g = c->b = 1.0f; c->a = 1.0f;
    if (!hex || hex[0] != '#' || strlen(hex) < 7) return;
    unsigned int ri = 0, gi = 0, bi = 0;
    sscanf(hex + 1, "%02x%02x%02x", &ri, &gi, &bi);
    c->r = (float)ri / 255.0f;
    c->g = (float)gi / 255.0f;
    c->b = (float)bi / 255.0f;
    snprintf(c->hex, sizeof(c->hex), "#%02x%02x%02x", ri, gi, bi);
}

GtkWidget *widget_colorbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_COLORBUTTON, NULL, "Color");

    parse_hex_color("#ffffff", &node->state.color);

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) parse_hex_color(def, &node->state.color);
    }
    return (GtkWidget *)node;
}

gchar *widget_colorbutton_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("#ffffff");
    /* Reconstruire hex depuis floats (render.cpp peut les modifier) */
    snprintf(n->state.color.hex, sizeof(n->state.color.hex), "#%02x%02x%02x",
             (int)(n->state.color.r * 255),
             (int)(n->state.color.g * 255),
             (int)(n->state.color.b * 255));
    return g_strdup(n->state.color.hex);
}
gchar *widget_colorbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_colorbutton_envvar_construct(var->Widget);
}
void widget_colorbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    parse_hex_color("#ffffff", &((WidgetNode *)var->Widget)->state.color);
}
void widget_colorbutton_refresh(variable *var) {}
void widget_colorbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_colorbutton_removeselected(variable *var) {}
void widget_colorbutton_save(variable *var) {}
