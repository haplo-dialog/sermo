/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_password.cpp — Champ mot de passe FLTK (Fl_Secret_Input)
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
#include "widget_password.h"
#include <FL/Fl_Secret_Input.H>
#include <string.h>

GtkWidget *widget_password_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int ww = 200, hh = 30;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request"))) ww = atoi(v);
    }
    Fl_Secret_Input *si = new Fl_Secret_Input(0, 0, ww, hh, nullptr);
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) si->value(def);
        el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) si->copy_label(lbl);
    }
    return (GtkWidget *)si;
}
gchar *widget_password_envvar_construct(GtkWidget *w)
{ return g_strdup(((Fl_Secret_Input *)w)->value() ? ((Fl_Secret_Input *)w)->value() : ""); }
gchar *widget_password_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_password_envvar_construct(v->Widget) : NULL; }
void widget_password_clear(variable *v)
{ if (v && v->Widget) ((Fl_Secret_Input *)v->Widget)->value(""); }
void widget_password_refresh(variable *v)
{ if (v && v->Widget) ((Fl_Secret_Input *)v->Widget)->redraw(); }
void widget_password_fileselect(variable *v, const char*, const char*) {}
void widget_password_removeselected(variable *v) {}
void widget_password_save(variable *v) {}
