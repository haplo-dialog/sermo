/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_linkbutton.c — Lien cliquable EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <linkbutton> : l'URI vient de <default>, le libellé de <label> (à défaut,
 * l'URI) — mêmes sources que l'étalon gtk3. Cliqué, il ouvre l'URI par
 * xdg-open lancé SANS shell (fork + execlp, URI en argv[1]) : « & » est banal
 * dans une URL de requête, et passer par /bin/sh rouvrirait une injection.
 *
 * Export : l'URI, comme l'étalon gtk3. Le stub exportait une chaîne VIDE.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_linkbutton.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define LINK_URI_KEY "sermo_uri"

/* Ouvre l'URI dans l'application par défaut, sans shell. Double fork : le
 * petit-fils est adopté par init, aucun zombie à récolter. */
static void _ouvrir_uri(const char *uri)
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
            _exit(127);
        }
        _exit(0);
    } else if (pid > 0) {
        int st;
        waitpid(pid, &st, 0);
    }
}

static void _link_clique(void *data, Evas_Object *obj, void *ev)
{
    (void) data; (void) ev;
    _ouvrir_uri(evas_object_data_get(obj, LINK_URI_KEY));
}

GtkWidget *widget_linkbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) attr; (void) Type;
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *btn = elm_button_add(parent ? parent
                                             : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
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

    /* Libellé BRUT : le thème Elementary d'un bouton n'interprète pas le
     * balisage — un « <u>…</u> » s'affichait tel quel à l'écran (mesuré en
     * capture). L'URI reste visible en info-bulle. */
    elm_object_text_set(btn, label);
    elm_object_tooltip_text_set(btn, uri);
    evas_object_data_set(btn, LINK_URI_KEY, g_strdup(uri));
    evas_object_smart_callback_add(btn, "clicked", _link_clique, NULL);
    evas_object_show(btn);
    return (GtkWidget *) btn;
}

gchar *widget_linkbutton_envvar_construct(GtkWidget *widget)
{
    const char *uri = widget ? evas_object_data_get((Evas_Object *) widget, LINK_URI_KEY)
                             : NULL;
    return g_strdup(uri ? uri : "");
}
gchar *widget_linkbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_linkbutton_envvar_construct(var->Widget);
}
void widget_linkbutton_clear(variable *var)          { (void) var; }
void widget_linkbutton_refresh(variable *var)        { (void) var; }
void widget_linkbutton_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_linkbutton_removeselected(variable *var) { (void) var; }
void widget_linkbutton_save(variable *var)           { (void) var; }
