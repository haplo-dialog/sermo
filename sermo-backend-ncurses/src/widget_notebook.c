/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_notebook.c — Widget notebook ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_notebook.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int g_sdl3_notebook_side = 0;
GtkWidget *widget_notebook_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    /* tab-pos="left" : memorise pour le rendu (barre verticale) */
    {
        const char *tp = attr ? get_tag_attribute(attr, "tab-pos") : NULL;
        g_sdl3_notebook_side = tp && (!strcasecmp(tp, "left") || !strcasecmp(tp, "right") ||
                                      !strcasecmp(tp, "start") || !strcmp(tp, "0"));
    }
    WidgetNode *node = widget_node_new(WT_NOTEBOOK, NULL, "");
    node->state.notebook.side = g_sdl3_notebook_side;
    node->state.notebook.current_tab = 0;

    /* Pop toutes les pages enfants */
    stackelement s;
    int i;
    s = pop();
    for (i = 0; i < s.nwidgets; i++) {
        if (s.widgets[i])
            widget_node_add_child(node, (WidgetNode *)s.widgets[i]);
    }

    /* Libelles d'onglets : attribut tab-labels="A|B|C" (parite etalon) */
    if (attr) {
        const char *tl = get_tag_attribute(attr, "tab-labels");
        if (tl && *tl) {
            gchar **ls = g_strsplit(tl, "|", -1);
            int n2 = 0; while (ls[n2]) n2++;
            node->state.notebook.tab_labels = ls;
            node->state.notebook.tab_count  = n2;
        }
    }
    return (GtkWidget *)node;
}

gchar *widget_notebook_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("0");
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", n->state.notebook.current_tab);
    return g_strdup(buf);
}

gchar *widget_notebook_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_notebook_envvar_construct(var->Widget);
}

void widget_notebook_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.notebook.current_tab = 0;
}

void widget_notebook_refresh(variable *var)
{
    /* <input> (commande ou fichier) : rejoué à chaque « refresh », comme
     * l'étalon gtk3sermo — sa PREMIÈRE ligne, CR/LF ôtés ; rien de lisible,
     * rien ne change ; aucune action ne joue. Jusqu'au 2026-10-13 il n'était
     * jamais lu. */
    /* atoi() de la ligne → page courante, règle de gtk_notebook_set_current_
     * page : négatif = dernière page, au-delà du nombre de pages = rien ne
     * change. Premier passage : voir plus bas. */
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    gchar *ligne = widget_input_ligne(var->Attributes);
    /* Premier passage : l'<input> pose la page, sauf si la balise porte
     * page="…", qui l'emporte — règle de gtk4sermo, et de gtk3sermo depuis le
     * 2026-10-18 (il perdait cette page, GtkNotebook la refusant avant
     * l'affichage). Jusque-là ce port copiait cette perte. */
    if (!n->initialised && (var->widget_tag_attr ? get_tag_attribute(var->widget_tag_attr, "page") : NULL)) {
        g_free(ligne);
        ligne = g_strdup((var->widget_tag_attr ? get_tag_attribute(var->widget_tag_attr, "page") : NULL));
    }
    if (ligne) {
        int p = atoi(ligne);
        if (p < 0) p = n->child_count - 1;
        if (p >= 0 && p < n->child_count) n->state.notebook.current_tab = p;
    }
    g_free(ligne);
    n->initialised = TRUE;
}
void widget_notebook_fileselect(variable *var, const char *name, const char *value) {}
void widget_notebook_removeselected(variable *var) {}
void widget_notebook_save(variable *var) {}
