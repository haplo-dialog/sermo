/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_searchentry.c — Champ de recherche ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ImGui::InputText avec placeholder "Rechercher..." et icône.
 * Le filtre est stocké dans state.list.filter et utilisé par
 * render.cpp pour filtrer les items d'un widget list associé.
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
#include "widget_searchentry.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_searchentry_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_SEARCHENTRY, NULL, "Search...");
    node->state.list.filter[0] = '\0';

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def)
            snprintf(node->state.list.filter, sizeof(node->state.list.filter), "%s", def);
    }
    return (GtkWidget *)node;
}

gchar *widget_searchentry_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("");
    return g_strdup(n->state.list.filter);
}
gchar *widget_searchentry_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_searchentry_envvar_construct(var->Widget);
}
void widget_searchentry_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.list.filter[0] = '\0';
}
void widget_searchentry_refresh(variable *var)
{
    /* <input> (commande ou fichier) : rejoué à chaque « refresh », comme
     * l'étalon gtk3sermo — sa PREMIÈRE ligne, CR/LF ôtés ; rien de lisible,
     * rien ne change ; aucune action ne joue. Jusqu'au 2026-10-13 il n'était
     * jamais lu. */
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    gchar *ligne = widget_input_ligne(var->Attributes);
    if (ligne) {
        snprintf(n->state.list.filter, sizeof(n->state.list.filter), "%s", ligne);
        g_free(ligne);
    }
    /* Premier passage : <default> l'emporte, l'étalon l'applique APRÈS
     * l'<input>. */
    if (!n->initialised) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        n->initialised = TRUE;
        if (def && *def)
            snprintf(n->state.list.filter, sizeof(n->state.list.filter), "%s", def);
    }
}
void widget_searchentry_fileselect(variable *var, const char *n, const char *v) {}
void widget_searchentry_removeselected(variable *var) {}
void widget_searchentry_save(variable *var) {}
