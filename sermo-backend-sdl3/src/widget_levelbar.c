/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_levelbar.c — Barre de niveau colorée SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Comme ProgressBar mais la couleur change selon le niveau :
 *   0.0–0.33 : rouge   (niveau bas)
 *   0.33–0.66: orange  (niveau moyen)
 *   0.66–1.0 : vert    (niveau élevé)
 * render.cpp adapte ImGui::PushStyleColor selon state.levelbar.level.
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
#include "widget_levelbar.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Le niveau (couleur) qui découle de la valeur. */
static void niveau_calculer(WidgetNode *node)
{
    double range = node->state.levelbar.max - node->state.levelbar.min;
    if (range > 0) {
        double ratio = (node->state.levelbar.value - node->state.levelbar.min) / range;
        node->state.levelbar.level = (ratio < 0.33) ? 0 : (ratio < 0.66) ? 1 : 2;
    }
}

GtkWidget *widget_levelbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_LEVELBAR, NULL, "");
    node->state.levelbar.value = 0.0;
    node->state.levelbar.min   = 0.0;
    node->state.levelbar.max   = 1.0;
    node->state.levelbar.level = 0;
    /* range-min / range-max (attributs de la balise, comme l'étalon) : ignorés
     * jusqu'à la 2.8.0, la barre se remplissait dès 1. */
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "range-min"))) node->state.levelbar.min = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-max"))) node->state.levelbar.max = g_ascii_strtod(v, NULL);
    }

    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def) node->state.levelbar.value = g_ascii_strtod(def, NULL);
    }
    /* <input> : lu par widget_levelbar_refresh(), que le cœur appelle juste
     * après la création — le lire aussi ici exécuterait la commande deux
     * fois. */
    niveau_calculer(node);
    return (GtkWidget *)node;
}

gchar *widget_levelbar_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("0");
    /* g_ascii_formatd et non snprintf : sous fr_FR, « %g » de 0.5 écrit
     * « 0,5 » et la valeur exportée dépendrait de la machine. */
    char buf[64];
    return g_strdup(g_ascii_formatd(buf, sizeof buf, "%g", n->state.levelbar.value));
}
gchar *widget_levelbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_levelbar_envvar_construct(var->Widget);
}
void widget_levelbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    n->state.levelbar.value = n->state.levelbar.min;
    n->state.levelbar.level = 0;
}
void widget_levelbar_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    /* <input> (commande ou fichier) : rejoué à chaque « refresh », comme
     * l'étalon gtk3sermo. Jusqu'au 2026-10-13 il n'était lu qu'à la création
     * et cette fonction était vide : une jauge pilotée par un <timer>
     * restait figée. */
    WidgetNode *n = (WidgetNode *)var->Widget;
    gchar *itext = widget_input_text(var->Attributes);
    if (itext) { n->state.levelbar.value = g_ascii_strtod(g_strchomp(itext), NULL); g_free(itext); }
    niveau_calculer(n);
}
void widget_levelbar_fileselect(variable *var, const char *nm, const char *v) {}
void widget_levelbar_removeselected(variable *var) {}
void widget_levelbar_save(variable *var) {}
