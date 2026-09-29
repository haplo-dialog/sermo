/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_timer.c — Widget timer SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * SDL3_TODO: le vrai timer SDL3 (SDL_AddTimer) sera câblé dans render.cpp.
 * Ce stub stocke l'intervalle et l'action dans le noeud.
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
#include "widget_timer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_timer_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_TIMER, NULL, "timer");

    /* label = intervalle NORMALISE en secondes (g_ascii_strtod : sous
     * locale fr, atof("0.5") rendrait 0). La render_loop le lit tel quel. */
    double interval = 1.0;
    char ibuf[32];
    if (Attr) {
        GList *el = NULL;
        gchar *act = attributeset_get_first(&el, Attr, ATTR_ACTION);
        if (act && *act)
            node->action = strdup(act);
        /* TOUTES les actions (l'unique node->action n'en gardait qu'une) */
        node->actions_attr = (void *)Attr;
    }
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "interval")) && *v)
            interval = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "milliseconds")) && *v)
            interval = g_ascii_strtod(v, NULL) / 1000.0;
        if ((v = get_tag_attribute(attr, "variable")) && *v)
            node->var_name = strdup(v);
    }
    if (interval <= 0) interval = 1.0;
    /* stocke en MILLISECONDES entieres : %d est insensible a la locale */
    snprintf(ibuf, sizeof(ibuf), "%d", (int)(interval * 1000.0));
    free(node->label);
    node->label = strdup(ibuf);

    return (GtkWidget *)node;
}

gchar *widget_timer_envvar_construct(GtkWidget *widget)
{
    /* Parite de VALEUR avec l'etalon : un timer exporte "true"/"false" */
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("false");
    return g_strdup("true");
}

gchar *widget_timer_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_timer_envvar_construct(var->Widget);
}

void widget_timer_clear(variable *var) {}
void widget_timer_refresh(variable *var) {}
void widget_timer_fileselect(variable *var, const char *name, const char *value) {}
void widget_timer_removeselected(variable *var) {}
void widget_timer_save(variable *var) {}
