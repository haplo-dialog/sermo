/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_eventbox.cpp — Conteneur captant les événements souris/clavier (FLTK)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <eventbox> → GtkEventBox : conteneur invisible qui enveloppe son contenu et
 * capte ses événements. Sous FLTK c'est un Fl_Flex transparent (FL_NO_BOX),
 * meme moteur de disposition que <vbox> : les enfants gardent leur taille.
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
#include "widget_eventbox.h"

#include <FL/Fl_Flex.H>

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_eventbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    /* Fl_Flex en colonne, comme <vbox> : l'eventbox enveloppe son contenu sans
     * le deformer. L'ancienne version prenait widgets[0] SEUL et le forcait a
     * 100x30 — un <frame> emballe disparaissait de l'ecran (mesure au banc
     * visuel). Elle ne depilait pas non plus les enfants suivants. */
    Fl_Flex *flex = new Fl_Flex(0, 0, 800, 600, Fl_Flex::COLUMN);
    flex->end();
    flex->box(FL_NO_BOX);
    flex->gap(0);
    flex->margin(0, 0);

    stackelement s = pop();
    int total = 0, added = 0;
    for (int n = 0; n < s.nwidgets; n++) {
        Fl_Widget *c = (Fl_Widget *) s.widgets[n];
        if (!c) continue;
        flex->add(c);
        if (!c->as_group()) flex->fixed(c, c->h());
        total += c->h();
        added++;
    }
    if (added > 0) {
        int wreq = 800;
        const char *wr = attr ? get_tag_attribute(attr, "width-request") : NULL;
        if (wr && atoi(wr) > 0) wreq = atoi(wr);
        flex->size(wreq, total);
    }
    return (GtkWidget *)flex;
}

gchar *widget_eventbox_envvar_construct(GtkWidget *widget)
{
    return g_strdup("");
}

gchar *widget_eventbox_envvar_all_construct(variable *var)
{
    return g_strdup("");
}

void widget_eventbox_clear(variable *var) {}

void widget_eventbox_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_eventbox_fileselect(variable *var, const char *n, const char *v) {}
void widget_eventbox_removeselected(variable *var) {}
void widget_eventbox_save(variable *var) {}
