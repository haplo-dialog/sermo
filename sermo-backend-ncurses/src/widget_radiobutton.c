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
#include "widget_radiobutton.h"
#include <string.h>
#include <stdlib.h>

/* Le bouton coché du groupe ouvert. Le groupe est celui de l'étalon gtk3 : le
 * cœur le ferme (lastradiowidget = NULL) dès qu'un autre widget est créé. */
static WidgetNode *coche_du_groupe;

GtkWidget *widget_radiobutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_RADIOBUTTON, NULL, "");
    node->state.radio.selected = 0;
    /* Premier bouton d'un groupe : coché, comme GTK (un groupe radio a toujours
     * un choix). */
    if (lastradiowidget == NULL) {
        lastradiowidget = (GtkWidget *)node;
        coche_du_groupe = node;
        node->state.radio.selected = 1;
    }
    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (strcasecmp(def,"true")==0 || strcmp(def,"1")==0)) {
            if (coche_du_groupe && coche_du_groupe != node) {
                coche_du_groupe->state.radio.selected = 0;
                sermo_be_bascule_jouer(coche_du_groupe, 0);   /* comme l'étalon */
            }
            coche_du_groupe = node;
            node->state.radio.selected = 1;
        }
    }
    /* Ses actions, jouées chaque fois qu'il bascule (clic ou « activate: ») :
     * voir jouer_actions_bascule(). Elles n'étaient pas gardées, donc jamais jouées. */
    if (Attr) {
        GList *ea = NULL;
        gchar *act = attributeset_get_first(&ea, Attr, ATTR_ACTION);
        if (act && *act) { node->action = strdup(act); node->actions_attr = (void *)Attr; }
    }
    return (GtkWidget *)node;
}

gchar *widget_radiobutton_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return g_strdup("false");
    return g_strdup(n->state.radio.selected ? "true" : "false");
}
gchar *widget_radiobutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_radiobutton_envvar_construct(var->Widget);
}
void widget_radiobutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.radio.selected = 0;
}
void widget_radiobutton_refresh(variable *var)
{
    WidgetNode *n;
    gint        v;
    GList      *el = NULL;
    gchar      *def;

    if (!var || !var->Widget) return;
    /* <input>, à chaque « refresh », comme l'étalon gtk3sermo : seul « vrai »
     * compte (un bouton radio ne se décoche pas seul, le groupe garde un
     * choix). Au premier passage, juste après la création, il coche sans jouer
     * ses actions — sauf si un <default> vrai l'a déjà fait — et le frère qu'il
     * remplace joue les siennes « décoché » ; ensuite, il choisit
     * comme un clic, et les actions jouent (le frère décoché d'abord).
     * Jusqu'au 2026-10-13, l'<input> d'un bouton radio n'était jamais lu. */
    n = (WidgetNode *)var->Widget;
    v = widget_input_bool(var->Attributes);
    if (!n->initialised) {
        n->initialised = TRUE;
        def = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        if (v == 1 && !(def && (strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))) {
            if (coche_du_groupe && coche_du_groupe != n) {
                coche_du_groupe->state.radio.selected = 0;
                sermo_be_bascule_jouer(coche_du_groupe, 0);   /* lui joue « décoché » */
            }
            coche_du_groupe = n;
            n->state.radio.selected = 1;
        }
        return;
    }
    if (v == 1 && !n->state.radio.selected)
        sermo_be_widget_activate(n);
}
void widget_radiobutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_radiobutton_removeselected(variable *var) {}
void widget_radiobutton_save(variable *var) {}
