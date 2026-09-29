/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_overlay.cpp — Enfants empilés (FLTK 1.4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * FLTK superpose naturellement : dans un Fl_Group, deux enfants de MÊME
 * géométrie se dessinent l'un sur l'autre, dans l'ordre d'ajout. Le premier
 * est donc le fond, exactement comme GtkOverlay. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_overlay.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Grid.H>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

GtkWidget *widget_overlay_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) attr; (void) Type;

    Fl_Group *grp = new Fl_Group(0, 0, 800, 200);
    grp->end();
    grp->box(FL_NO_BOX);

    stackelement s = pop();
    int poses = 0, haut = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        Fl_Widget *c = (Fl_Widget *) s.widgets[n];
        if (!c) continue;
        if (c->h() > haut) haut = c->h();
        grp->add(c);
        poses++;
    }
    if (haut < 20) haut = 20;
    grp->size(800, haut);
    /* Même rectangle pour tous : c'est CE choix qui superpose. */
    for (int i = 0; i < grp->children(); ++i) grp->child(i)->resize(0, 0, 800, haut);
    if (poses < 2)
        fprintf(stderr, "fltk1sermo: <overlay> n'a reçu que %d enfant(s) : il n'y "
                        "a rien à superposer.\n", poses);
    return (GtkWidget *) grp;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_overlay_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_overlay_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_overlay_envvar_construct(var->Widget);
}
void widget_overlay_clear(variable *var)   { (void) var; }
void widget_overlay_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *) var->Widget)->redraw();
}
void widget_overlay_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_overlay_removeselected(variable *var) { (void) var; }
void widget_overlay_save(variable *var)           { (void) var; }
