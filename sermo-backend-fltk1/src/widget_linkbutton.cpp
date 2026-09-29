/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_linkbutton.cpp — Lien cliquable (portage FLTK)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <linkbutton> : l'URI vient de <default>, le libellé de <label> (à défaut,
 * l'URI) — mêmes sources que l'étalon gtk3. L'ancienne version ne lisait PAS
 * l'URI, ne faisait rien au clic, et exportait une chaîne VIDE (écart mesuré
 * au banc de comportement, cas 26).
 *
 * Le clic ouvre l'URI par xdg-open lancé SANS shell (fork + execlp, URI en
 * argv[1]) : « & » est banal dans une URL de requête, et passer par /bin/sh
 * rouvrirait une injection. FLTK propose bien fl_open_uri(), mais il construit
 * une ligne de commande et la confie à un shell — on s'en passe.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_linkbutton.h"
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>

namespace {

/* Ouvre l'URI dans l'application par défaut, sans shell. Double fork : le
 * petit-fils est adopté par init, aucun zombie à récolter. */
void ouvrir_uri(const char *uri)
{
    if (!uri || !*uri) return;
    if (uri[0] == '-') {           /* serait pris pour une option par xdg-open */
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

/* Bouton qui porte SON URI : c'est elle que le dialogue exporte, et elle ne
 * doit pas dépendre du libellé affiché. */
class SermoLink : public Fl_Button {
public:
    SermoLink(int x, int y, int w, int h, const char *uri)
        : Fl_Button(x, y, w, h, nullptr), uri_(uri ? uri : "")
    {
        callback(clic, this);
    }
    const std::string &uri() const { return uri_; }

private:
    static void clic(Fl_Widget *, void *v)
    {
        SermoLink *l = static_cast<SermoLink *>(v);
        ouvrir_uri(l->uri_.c_str());
    }
    std::string uri_;
};

} /* namespace */

GtkWidget *widget_linkbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;
    int w = 120, h = 30;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

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
    if (!label) label = uri;       /* étalon : sans <label>, l'URI fait office */

    SermoLink *wdg = new SermoLink(0, 0, w, h, uri);
    wdg->copy_label(label);
    wdg->labelcolor(FL_BLUE);      /* un lien doit se voir comme un lien */
    return (GtkWidget *) wdg;
}

gchar *widget_linkbutton_envvar_construct(GtkWidget *widget)
{
    SermoLink *l = dynamic_cast<SermoLink *>((Fl_Widget *) widget);
    return g_strdup(l ? l->uri().c_str() : "");
}
gchar *widget_linkbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_linkbutton_envvar_construct(var->Widget);
}
void widget_linkbutton_clear(variable *var) { (void) var; }
void widget_linkbutton_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *) var->Widget)->redraw();
}
void widget_linkbutton_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_linkbutton_removeselected(variable *var) { (void) var; }
void widget_linkbutton_save(variable *var)           { (void) var; }
