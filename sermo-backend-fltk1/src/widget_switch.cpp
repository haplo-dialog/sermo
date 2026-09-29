/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_switch.cpp — Interrupteur booléen FLTK (Fl_Light_Button)
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
#include "widget_switch.h"
#include <FL/Fl_Light_Button.H>
#include <string.h>

GtkWidget *widget_switch_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Fl_Light_Button *btn = new Fl_Light_Button(0, 0, 120, 30, "OFF");
    btn->color2(fl_rgb_color(137, 180, 250));  /* Catppuccin blue */
    btn->callback([](Fl_Widget *w, void*){
        Fl_Light_Button *b = (Fl_Light_Button *)w;
        b->copy_label(b->value() ? "ON" : "OFF");
    });
    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) btn->copy_label(lbl);
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (strcmp(def,"true")==0||strcmp(def,"1")==0||strcmp(def,"on")==0))
            btn->value(1);
    }
    return (GtkWidget *)btn;
}
gchar *widget_switch_envvar_construct(GtkWidget *w)
{ return g_strdup(((Fl_Light_Button *)w)->value() ? "true" : "false"); }
gchar *widget_switch_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_switch_envvar_construct(v->Widget) : NULL; }
void widget_switch_clear(variable *v)
{ if (v && v->Widget) { ((Fl_Light_Button *)v->Widget)->value(0); ((Fl_Light_Button *)v->Widget)->copy_label("OFF"); } }
void widget_switch_refresh(variable *v)
{ if (v && v->Widget) ((Fl_Light_Button *)v->Widget)->redraw(); }
void widget_switch_fileselect(variable *v, const char*, const char*) {}
void widget_switch_removeselected(variable *v) {}
void widget_switch_save(variable *v) {}
