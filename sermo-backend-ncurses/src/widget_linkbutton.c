/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_linkbutton.c — Lien cliquable ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <linkbutton> : l'URI vient de <default>, le libellé de <label> (à défaut,
 * l'URI elle-même) — mêmes sources que l'étalon gtk3 (widget_linkbutton.c,
 * gtk_link_button_new_with_label). Activé (Entrée/Espace), il ouvre l'URI par
 * xdg-open SANS shell ; en terminal il reste utile même sans session graphique
 * puisque la valeur exportée, elle, est toujours l'URI.
 *
 * Export : l'URI, comme l'étalon gtk3 (gtk_link_button_get_uri). Le stub
 * précédent exportait une chaîne VIDE — écart mesuré sur le banc.
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
#include "widget_linkbutton.h"
#include "sermo_open_uri.h"
#include <stdlib.h>
#include "sermo_lanceur.h"

/* Ouvre l'URI dans l'application par défaut, sans passer par un shell.
 * Le lanceur unique du cœur détache l'enfant : aucun zombie à récolter. */
void sermo_open_uri(const char *uri)
{
    if (!uri || !*uri) return;
    if (uri[0] == '-') {            /* serait pris pour une option par xdg-open */
        fprintf(stderr, "sermo: URI refusée (commence par « - ») : %s\n", uri);
        return;
    }
#ifdef _WIN32
    /* Windows : ShellExecute LANCERAIT un .exe qu'on lui désigne, ce que
     * xdg-open ne fait pas. Seules les adresses http, https et mailto
     * passent (sermo_win32.c). */
    if (sermo_win32_ouvrir_uri(uri) != 0)
        fprintf(stderr, "sermo: lien refusé ou impossible à ouvrir sous Windows "
                "(seuls http, https et mailto) : %s\n", uri);
#else
    char *argv[] = { "xdg-open", (char *) uri, NULL };
    char *erreur = NULL;
    if (sermo_lance_async(argv, NULL, NULL, NULL, &erreur) != 0) {
        fprintf(stderr, "sermo: xdg-open n'a pas pu être lancé : %s\n", erreur ? erreur : "?");
        free(erreur);
    }
#endif
}

GtkWidget *widget_linkbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) attr; (void) Type;
    const char *uri = NULL, *label = NULL;

    if (Attr) {
        GList *el = NULL;
        gchar *d = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (d) uri = d;
        el = NULL;
        gchar *l = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (l && *l) label = l;
    }
    if (!uri) uri = "";
    if (!label) label = uri;        /* étalon : sans <label>, l'URI fait office */

    WidgetNode *node = widget_node_new(WT_LINKBUTTON, NULL, label);
    snprintf(node->state.entry.buf, sizeof(node->state.entry.buf), "%s", uri);
    return (GtkWidget *) node;
}

gchar *widget_linkbutton_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *) widget;
    if (!n) return g_strdup("");
    return g_strdup(n->state.entry.buf);
}
gchar *widget_linkbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_linkbutton_envvar_construct(var->Widget);
}
void widget_linkbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *) var->Widget)->state.entry.buf[0] = '\0';
}
void widget_linkbutton_refresh(variable *var)
{
    /* <input> (commande ou fichier) : rejoué à chaque « refresh », comme
     * l'étalon gtk3sermo — sa PREMIÈRE ligne, CR/LF ôtés ; rien de lisible,
     * rien ne change ; aucune action ne joue. Jusqu'au 2026-10-13 il n'était
     * jamais lu. */
    /* La ligne devient l'URI (gtk_link_button_set_uri), PAS le libellé :
     * celui-ci reste ce que create a posé, comme chez l'étalon. */
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *) var->Widget;
    gchar *ligne = widget_input_ligne(var->Attributes);
    if (ligne) {
        snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", ligne);
        g_free(ligne);
    }
}
void widget_linkbutton_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_linkbutton_removeselected(variable *var) { (void) var; }
void widget_linkbutton_save(variable *var)           { (void) var; }
