/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
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
#include "widget_expander.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>


/* Même règle que l'étalon (gtk3 widget_expander.c) : l'état initial vient de
 * l'ATTRIBUT DE BALISE expanded= — « true », « yes » ou 1 — et un expander sans
 * cet attribut est REPLIÉ. Ce port lisait <default> (que l'étalon ignore) et
 * partait ouvert : le cas 40 du banc mesure les deux écarts. */
static gboolean expander_ouvert_au_depart(tag_attr *attr)
{
    const char *v = attr ? get_tag_attribute(attr, "expanded") : NULL;
    if (!v) return FALSE;
    return (strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 || atoi(v) == 1);
}

GtkWidget *widget_expander_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_EXPANDER, NULL, "");
    node->state.expander.open = expander_ouvert_au_depart(attr);
    node->state.expander.impose = (attr && get_tag_attribute(attr, "expanded"));
    node->state.expander.ouvert_impose = node->state.expander.open;
    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
    }
    /* Le titre s'écrit aussi en attribut, <expander label="…"> : l'étalon gtk3 le
     * lit (propriété GtkExpander:label), ce port l'ignorait (mesuré sur les
     * captures du site, 2026-10-10). */
    if (attr) {
        const char *v = get_tag_attribute(attr, "label");
        if (v && *v) { free(node->label); node->label = strdup(v); }
    }
    stackelement s = pop();
    if (s.widgets[0]) widget_node_add_child(node, (WidgetNode *)s.widgets[0]);
    return (GtkWidget *)node;
}

gchar *widget_expander_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return g_strdup("false");
    return g_strdup(n->state.expander.open ? "true" : "false");
}
gchar *widget_expander_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_expander_envvar_construct(var->Widget);
}
void widget_expander_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.expander.open = FALSE;
}
void widget_expander_refresh(variable *var)
{
    WidgetNode *n;
    gint        v;

    if (!var || !var->Widget) return;
    /* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo (sa
     * première ligne : true/yes/1 → déplié, autre chose → replié), sans jouer
     * d'action. Au premier passage, l'attribut expanded= l'emporte : l'étalon
     * pose les attributs de balise après (mesuré : expanded="true" et
     * <input> « non » → « true »). Jusqu'au 2026-10-13 il n'était jamais lu. */
    n = (WidgetNode *)var->Widget;
    v = widget_input_bool(var->Attributes);
    if (v >= 0) n->state.expander.open = v;
    if (!n->initialised) {
        n->initialised = TRUE;
        if (n->state.expander.impose)
            n->state.expander.open = n->state.expander.ouvert_impose;
    }
}
void widget_expander_fileselect(variable *var, const char *n, const char *v) {}
void widget_expander_removeselected(variable *var) {}
void widget_expander_save(variable *var) {}
