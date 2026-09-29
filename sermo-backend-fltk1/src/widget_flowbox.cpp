/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_flowbox.cpp — Rangement en lignes (FLTK 1.4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ FLTK n'a pas de disposition en flot : Fl_Flex ne repasse pas à la ligne.
 * Comme pour Qt, les enfants sont rangés dans une GRILLE à
 * « max-children-per-line » colonnes (Fl_Grid, nouveau en 1.4) — même rendu
 * tant que la fenêtre ne rétrécit pas, sans reflux dynamique.
 *
 * ⚠️ Pas de sélection : l'export rend toujours la chaîne vide, ce que l'étalon
 * rend aussi tant que rien n'est sélectionné. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_flowbox.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Grid.H>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

GtkWidget *widget_flowbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    int par_ligne = 4, esp_col = 6, esp_lig = 6;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "max-children-per-line"))) par_ligne = atoi(v);
        if ((v = get_tag_attribute(attr, "column-spacing")))        esp_col   = atoi(v);
        if ((v = get_tag_attribute(attr, "row-spacing")))           esp_lig   = atoi(v);
    }
    if (par_ligne < 1) par_ligne = 1;

    Fl_Grid *grille = new Fl_Grid(0, 0, 800, 200);
    grille->end();
    grille->box(FL_NO_BOX);

    stackelement s = pop();
    int retenus = 0;
    for (int n = 0; n < s.nwidgets; ++n) if (s.widgets[n]) retenus++;
    int rangees = retenus ? (retenus + par_ligne - 1) / par_ligne : 1;
    grille->layout(rangees, par_ligne, 0, 0);
    grille->row_gap(esp_lig);
    grille->col_gap(esp_col);

    int i = 0, haut = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        Fl_Widget *c = (Fl_Widget *) s.widgets[n];
        if (!c) continue;
        grille->add(c);
        grille->widget(c, i / par_ligne, i % par_ligne);
        if (i % par_ligne == 0) haut += c->h() + esp_lig;
        i++;
    }
    if (haut > 0) grille->size(800, haut);
    return (GtkWidget *) grille;
}

/* Export : l'index de l'enfant sélectionné — ce port n'en sélectionne aucun. */
gchar *widget_flowbox_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_flowbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_flowbox_envvar_construct(var->Widget);
}
void widget_flowbox_clear(variable *var)   { (void) var; }
void widget_flowbox_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *) var->Widget)->redraw();
}
void widget_flowbox_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_flowbox_removeselected(variable *var) { (void) var; }
void widget_flowbox_save(variable *var)           { (void) var; }
