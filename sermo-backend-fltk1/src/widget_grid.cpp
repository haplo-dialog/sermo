/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_grid.cpp — Conteneur de mise en page en tableau (FLTK 1.4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <grid columns="N"> : Fl_Grid, nouveau dans FLTK 1.4 — c'est exactement le
 * conteneur qu'il faut, inutile de calculer des positions à la main.
 * Remplissage EN FLOT (ordre du document, retour à la ligne tous les N).
 *
 * ⚠️ Ordre d'appel obligatoire : end() AVANT d'ajouter, sinon le groupe
 * capture les widgets créés ensuite ailleurs dans le programme ; puis add(),
 * puis layout(rangées, colonnes), puis widget(enfant, r, c).
 *
 * ⚠️ <grid> n'est pas <table> : <table> est la liste à colonnes des données.
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
#include "widget_grid.h"
#include <FL/Fl.H>
#include <FL/Fl_Grid.H>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int grid_attr_int(tag_attr *attr, const char *nom, int repli)
{
    if (!attr) return repli;
    const char *v = get_tag_attribute(attr, nom);
    if (!v || !*v) return repli;
    int n = atoi(v);
    return (n >= 0) ? n : repli;
}

GtkWidget *widget_grid_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    int columns = grid_attr_int(attr, "columns", 0);
    if (columns <= 0) {
        fprintf(stderr, "fltk1sermo: <grid> sans attribut columns= utilisable : "
                        "une seule colonne.\n");
        columns = 1;
    }
    const int ecart_r = grid_attr_int(attr, "row-spacing", 4);
    const int ecart_c = grid_attr_int(attr, "column-spacing", 8);

    Fl_Grid *grid = new Fl_Grid(0, 0, 800, 600);
    grid->end();                    /* ne rien capturer d'autre */
    grid->box(FL_NO_BOX);

    stackelement s = pop();

    /* Compter d'abord : le nombre de rangées dépend du nombre d'enfants
     * RETENUS (un enfant nul ne compte pas), et layout() veut ce compte
     * AVANT qu'on assigne les cellules. */
    int retenus = 0;
    for (int n = 0; n < s.nwidgets; ++n)
        if (s.widgets[n]) retenus++;
    const int rangees = retenus ? (retenus + columns - 1) / columns : 1;

    grid->layout(rangees, columns, 0, 0);
    grid->row_gap(ecart_r);
    grid->col_gap(ecart_c);

    int hauteur = 0, i = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        Fl_Widget *c = (Fl_Widget *) s.widgets[n];
        if (!c) continue;
        grid->add(c);
        grid->widget(c, i / columns, i % columns);
        if (i % columns == 0) hauteur += c->h() + ecart_r;
        i++;
    }

    /* Hauteur naturelle : sans elle la grille garde 600 px et pousse le reste
     * du dialogue hors de la fenêtre (même piège que <vbox> sur ce port). */
    if (hauteur > 0) grid->size(800, hauteur);

    return (GtkWidget *) grid;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_grid_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_grid_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_grid_envvar_construct(var->Widget);
}
void widget_grid_clear(variable *var)   { (void) var; }
void widget_grid_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *) var->Widget)->redraw();
}
void widget_grid_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_grid_removeselected(variable *var) { (void) var; }
void widget_grid_save(variable *var)           { (void) var; }
