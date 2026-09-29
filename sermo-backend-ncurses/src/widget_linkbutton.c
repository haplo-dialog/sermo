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
#include <unistd.h>
#include <sys/wait.h>

/* Ouvre l'URI dans l'application par défaut, sans passer par un shell.
 * Double fork : le petit-fils est adopté par init, aucun zombie à récolter. */
void sermo_open_uri(const char *uri)
{
    if (!uri || !*uri) return;
    if (uri[0] == '-') {            /* serait pris pour une option par xdg-open */
        fprintf(stderr, "sermo: URI refusée (commence par « - ») : %s\n", uri);
        return;
    }
    pid_t pid = fork();
    if (pid == 0) {
        pid_t petit = fork();
        if (petit == 0) {
            execlp("xdg-open", "xdg-open", uri, (char *) NULL);
            _exit(127);             /* xdg-open absent : échec silencieux */
        }
        _exit(0);
    } else if (pid > 0) {
        int st;
        waitpid(pid, &st, 0);       /* le fils intermédiaire sort tout de suite */
    }
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
void widget_linkbutton_refresh(variable *var)        { (void) var; }
void widget_linkbutton_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_linkbutton_removeselected(variable *var) { (void) var; }
void widget_linkbutton_save(variable *var)           { (void) var; }
