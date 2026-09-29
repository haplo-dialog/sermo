/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_fontbutton.cpp — Bouton de sélection de police FLTK
 * sermo — haplo-dialog — GPL-2.0-or-later
 * Implémenté via safe_popen("fc-list : family") + Fl_Choice.
 * Export : nom de police sélectionnée. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "safe_exec.h"
#include "widget_fontbutton.h"
#include <FL/Fl_Choice.H>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const int FONT_MAX = 256;

GtkWidget *widget_fontbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Fl_Choice *ch = new Fl_Choice(0, 0, 200, 30);

    /* Le <default> ("Sans 12") est la valeur de VÉRITÉ : posé en tête et
     * sélectionné tel quel — parité étalon, qui rend la chaîne de police
     * telle que fournie. */
    gchar *def = NULL;
    if (Attr) {
        GList *el = NULL;
        def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
    }
    ch->add(def && *def ? def : "Sans");

    /* Familles système par l'API NATIVE FLTK (Fl::set_fonts) : zéro
     * sous-processus — l'ancien fc-list coûtait un spawn par création
     * (et sa première version à tubes déclenchait le repli shell). */
    {
        int nf = Fl::set_fonts(NULL);
        int n = 0;
        for (int i = 0; i < nf && n < FONT_MAX; i++) {
            int attrs = 0;
            const char *name = Fl::get_font_name((Fl_Font)i, &attrs);
            if (name && *name && !attrs && !ch->find_item(name)) {
                ch->add(name);
                ++n;
            }
        }
    }
    ch->value(0);
    return (GtkWidget *)ch;
}
gchar *widget_fontbutton_envvar_construct(GtkWidget *w)
{
    Fl_Choice *c = (Fl_Choice *)w;
    const Fl_Menu_Item *it = c->mvalue();
    return g_strdup(it && it->label() ? it->label() : "Sans");
}
gchar *widget_fontbutton_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_fontbutton_envvar_construct(v->Widget) : NULL; }
void widget_fontbutton_clear(variable *v)
{ if (v && v->Widget) ((Fl_Choice *)v->Widget)->value(0); }
void widget_fontbutton_refresh(variable *v)
{ if (v && v->Widget) ((Fl_Choice *)v->Widget)->redraw(); }
void widget_fontbutton_fileselect(variable *v, const char*, const char*) {}
void widget_fontbutton_removeselected(variable *v) {}
void widget_fontbutton_save(variable *v) {}
