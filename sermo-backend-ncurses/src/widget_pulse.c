/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_pulse.c — Barre d'activité indéterminée ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <pulse> : barre de progression SANS valeur — elle signale « ça travaille »,
 * pas « on en est à 42 % ». L'étalon gtk3 crée un GtkProgressBar et honore les
 * attributs de balise text= et show-text= ; <default> fournit le texte initial.
 * En terminal, l'animation est un bloc qui va et vient dans la barre : la phase
 * vit dans state.spinner.angle, avancée à chaque frame par le renderer.
 *
 * Export : la chaîne fixe « pulse », comme l'étalon gtk3. Le stub précédent
 * exportait une chaîne VIDE — écart mesuré sur le banc.
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
#include "widget_pulse.h"
#include <stdlib.h>

GtkWidget *widget_pulse_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;
    const char *text = NULL;
    WidgetNode *node = widget_node_new(WT_PULSE, NULL, "");
    node->state.spinner.angle = 0.0f;      /* phase de l'animation */
    node->state.spinner.speed = 1.0;       /* pas par frame */

    if (attr) {
        const char *t = get_tag_attribute(attr, "text");
        if (t && *t) text = t;
    }
    if (!text && Attr) {                   /* <default> : texte initial */
        GList *el = NULL;
        gchar *d = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (d && *d) text = d;
    }
    if (text) { free(node->label); node->label = strdup(text); }
    return (GtkWidget *) node;
}

gchar *widget_pulse_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("pulse");              /* étalon gtk3 : valeur fixe */
}
gchar *widget_pulse_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_pulse_envvar_construct(var->Widget);
}
void widget_pulse_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *) var->Widget)->state.spinner.angle = 0.0f;
}
void widget_pulse_refresh(variable *var)        { (void) var; }
void widget_pulse_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_pulse_removeselected(variable *var) { (void) var; }
void widget_pulse_save(variable *var)           { (void) var; }
