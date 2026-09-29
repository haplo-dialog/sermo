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
#include "widget_edit.h"
#include "safe_exec.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_edit_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_EDIT, NULL, "");
    node->state.text.content = strdup("");
    node->state.text.len = 0;

    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) { free(node->state.text.content);
            node->state.text.content = strdup(def); }
        /* <input> : lu par widget_edit_refresh(), que le cœur appelle juste
         * après la création — le lire aussi ici exécuterait la commande deux
         * fois. */
    }
    node->state.text.len = (int)strlen(node->state.text.content);
    /* background / foreground (extension sermo) : #rrggbb -> ImGui */
    if (attr) {
        const char *bg = get_tag_attribute(attr, "background");
        const char *fg = get_tag_attribute(attr, "foreground");
        unsigned v;
        if (bg && bg[0] == '#' && sscanf(bg + 1, "%6x", &v) == 1) node->bg_rgba = 0xff000000u | v;
        if (fg && fg[0] == '#' && sscanf(fg + 1, "%6x", &v) == 1) node->fg_rgba = 0xff000000u | v;
        const char *ex = get_tag_attribute(attr, "space-expand");
        if (ex && (strcasecmp(ex, "true") == 0 || strcasecmp(ex, "yes") == 0 || strcmp(ex, "1") == 0))
            node->expand = 1;
    }
    return (GtkWidget *)node;
}

gchar *widget_edit_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return g_strdup("");
    return g_strdup(n->state.text.content ? n->state.text.content : "");
}
gchar *widget_edit_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_edit_envvar_construct(var->Widget);
}
void widget_edit_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    free(n->state.text.content); n->state.text.content = strdup(""); n->state.text.len = 0;
    /* cap = taille RÉELLE du nouveau tampon (1 octet). Le rendu sdl3 confie
     * `cap` à ImGui comme taille d'écriture : laissé à 8 192 après un premier
     * affichage, la saisie qui suit un clear: écrirait hors du tampon. */
    n->state.text.cap = 1;
}
void widget_edit_refresh(variable *var)
{
    if (!var || !var->Widget || !var->Attributes) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    GList *el = NULL;
    /* <input> (commande ou fichier) : le contenu ENTIER, saut de ligne final
     * compris — règle de l'étalon gtk3sermo. */
    gchar *text = widget_input_text(var->Attributes);
    /* Au premier refresh, l'étalon pose <default> APRÈS <input> : le texte
     * par défaut, déjà mis par la création, l'emporte (mesuré) — la commande
     * s'exécute quand même. */
    gchar *def = n->initialised ? NULL
               : attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
    if (text && !(def && *def)) {
        free(n->state.text.content);
        n->state.text.content = strdup(text);
        n->state.text.len = (int)strlen(n->state.text.content);
        /* cap = taille RÉELLE du tampon : le rendu s'y fie avant d'y laisser
         * écrire, et l'agrandit s'il le faut. */
        n->state.text.cap = n->state.text.len + 1;
    }
    g_free(text);
    n->initialised = TRUE;
}
void widget_edit_fileselect(variable *var, const char *n, const char *v) {}
void widget_edit_removeselected(variable *var) {}
void widget_edit_save(variable *var) {}
