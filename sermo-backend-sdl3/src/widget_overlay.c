/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_overlay.c — Enfants empilés
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ Un terminal ne superpose pas, et l'immediate-mode non plus sans travail
 * de curseur. Le renderer dessine le fond PUIS les couches, l'une sous
 * l'autre : le contenu reste visible et rien n'est perdu — c'est la
 * dégradation honnête, elle est écrite dans le manuel. */
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
#include "widget_overlay.h"
#include <stdlib.h>

GtkWidget *widget_overlay_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) attr; (void) Type;
    WidgetNode *node = widget_node_new(WT_OVERLAY, NULL, "");
    int poses = 0;

    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i) {
        if (!s.widgets[i]) continue;
        widget_node_add_child(node, (WidgetNode *) s.widgets[i]);
        poses++;
    }
    if (poses < 2)
        fprintf(stderr, "sermo: <overlay> n'a reçu que %d enfant(s) : il n'y a "
                        "rien à superposer.\n", poses);
    return (GtkWidget *) node;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_overlay_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_overlay_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_overlay_envvar_construct(var->Widget);
}
void widget_overlay_clear(variable *var)          { (void) var; }
void widget_overlay_refresh(variable *var)        { (void) var; }
void widget_overlay_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_overlay_removeselected(variable *var) { (void) var; }
void widget_overlay_save(variable *var)           { (void) var; }
