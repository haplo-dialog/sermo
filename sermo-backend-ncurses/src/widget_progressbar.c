/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
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
#include "widget_progressbar.h"
#include "sermo_progress.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void noeud_barre_fraction(void *widget, double fraction)
{
    ((WidgetNode *)widget)->state.progress.value = fraction;
}

static void noeud_barre_texte(void *widget, const char *texte)
{
    WidgetNode *node = (WidgetNode *)widget;
    free(node->label);
    node->label = strdup(texte);
}

GtkWidget *widget_progressbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_PROGRESSBAR, NULL, "");
    node->state.progress.value = 0.0;
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) node->state.progress.value = g_ascii_strtod(def, NULL) / 100.0;
        el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
    }
    /* La commande <input> est lue AU FIL DE L'EAU par le cœur
     * (sermo_progress.h), relevée par la boucle du port. Jusqu'à la 2.6.8,
     * ce port la lisait EN ENTIER avant d'ouvrir la fenêtre, puis posait un
     * seul nombre : la barre ne progressait jamais et l'action prévue à 100
     * ne partait pas. */
    node->lecture = sermo_progress_start(node, Attr, noeud_barre_fraction, noeud_barre_texte);
    return (GtkWidget *)node;
}

gchar *widget_progressbar_envvar_construct(GtkWidget *w)
{
    /* L'étalon gtk3 n'exporte RIEN pour une barre de progression : c'est un
     * afficheur, pas une saisie. Ce port rendait un pourcentage, qu'un script
     * écrit pour gtk3 ne pouvait pas attendre. */
    (void) w;
    return g_strdup("");
}
gchar *widget_progressbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_progressbar_envvar_construct(var->Widget);
}
void widget_progressbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.progress.value = 0.0;
}
void widget_progressbar_refresh(variable *var) {}
void widget_progressbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_progressbar_removeselected(variable *var) {}
void widget_progressbar_save(variable *var) {}
