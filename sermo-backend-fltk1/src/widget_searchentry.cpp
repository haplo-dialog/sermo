/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_searchentry.cpp — Champ de recherche FLTK (Fl_Input avec label "🔍")
 * sermo — haplo-dialog — GPL-2.0-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_searchentry.h"
#include <FL/Fl_Input.H>
#include <string.h>

GtkWidget *widget_searchentry_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int ww = 200, hh = 30;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request"))) ww = atoi(v);
    }
    Fl_Input *inp = new Fl_Input(0, 0, ww, hh);   /* pas de label externe : FLTK le dessine HORS du champ, sur le voisin */
    inp->value("");
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) inp->value(def);
    }
    return (GtkWidget *)inp;
}
gchar *widget_searchentry_envvar_construct(GtkWidget *w)
{ return g_strdup(((Fl_Input *)w)->value() ? ((Fl_Input *)w)->value() : ""); }
gchar *widget_searchentry_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_searchentry_envvar_construct(v->Widget) : NULL; }
void widget_searchentry_clear(variable *v)
{ if (v && v->Widget) ((Fl_Input *)v->Widget)->value(""); }
void widget_searchentry_refresh(variable *v)
{ if (v && v->Widget) ((Fl_Input *)v->Widget)->redraw(); }
void widget_searchentry_fileselect(variable *v, const char*, const char*) {}
void widget_searchentry_removeselected(variable *v) {}
void widget_searchentry_save(variable *v) {}
