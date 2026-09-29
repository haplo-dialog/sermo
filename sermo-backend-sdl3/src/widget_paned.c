/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_paned.c — Deux zones et une poignée déplaçable
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le nœud porte les deux enfants et la description de la poignée ; c'est le
 * renderer du port qui découpe et laisse déplacer (render_*.c, WT_PANED).
 * Étalon = gtk3 (GtkPaned).
 *
 * ⚠️ EXACTEMENT deux enfants ; un troisième est refusé AVEC un message — le
 * taire referait le défaut d'<eventbox>, qui perdait son contenu sans rien dire.
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
#include "stack.h"
#include "widget_paned.h"
#include <stdlib.h>

GtkWidget *widget_paned_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;
    WidgetNode *node = widget_node_new(WT_PANED, NULL, "");

    node->state.paned.vertical    = FALSE;
    node->state.paned.fraction    = 0.5;   /* moitié-moitié par défaut */
    node->state.paned.pixels      = 0;
    node->state.paned.resizable   = TRUE;

    if (attr) {
        const char *v = get_tag_attribute(attr, "orientation");
        if (v && !strcasecmp(v, "vertical")) node->state.paned.vertical = TRUE;

        v = get_tag_attribute(attr, "resizable");
        if (v && (!strcasecmp(v, "false") || !strcasecmp(v, "no") || !strcmp(v, "0")))
            node->state.paned.resizable = FALSE;

        v = get_tag_attribute(attr, "position");
        if (v && *v) {
            char *fin = NULL;
            /* ⛔ g_ascii_strtod, jamais strtod ni atof : sous une locale
             * française, « 0.3 » lu par strtod rend ZÉRO en silence. La garde
             * tests/garde_fonctions_interdites.sh refuse les deux autres. */
            double d = g_ascii_strtod(v, &fin);
            if (fin && *fin == '%') {
                if (d > 0 && d < 100) node->state.paned.fraction = d / 100.0;
            } else if (d >= 1) {
                node->state.paned.pixels = (int) d;
            }
        }
    }

    /* Un seul pop : le cœur coalesce les enfants (instruction SUM). */
    stackelement s = pop();
    int retenus = 0;
    for (int i = 0; i < s.nwidgets; ++i) {
        if (!s.widgets[i]) continue;
        if (retenus >= 2) {
            fprintf(stderr, "sermo: <paned> prend EXACTEMENT deux enfants : le %de "
                            "est ignoré. Emballer le surplus dans une <vbox>.\n",
                    retenus + 1);
            retenus++;
            continue;
        }
        widget_node_add_child(node, (WidgetNode *) s.widgets[i]);
        retenus++;
    }
    if (retenus < 2)
        fprintf(stderr, "sermo: <paned> n'a reçu que %d enfant(s) : la poignée n'a "
                        "rien à partager.\n", retenus);

    return (GtkWidget *) node;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_paned_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_paned_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_paned_envvar_construct(var->Widget);
}
void widget_paned_clear(variable *var)          { (void) var; }
void widget_paned_refresh(variable *var)        { (void) var; }
void widget_paned_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_paned_removeselected(variable *var) { (void) var; }
void widget_paned_save(variable *var)           { (void) var; }
