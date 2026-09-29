/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_paned.cpp — Deux zones et une poignée déplaçable (FLTK 1.4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <paned> : Fl_Tile. FLTK n'a pas de « poignée » dessinée — c'est la FRONTIÈRE
 * entre les deux enfants qui se saisit à la souris. Conséquence pratique :
 * contrairement aux autres ports, les enfants doivent être POSITIONNÉS à la
 * main pour couvrir toute la surface du Fl_Tile ; un enfant qui ne la couvre
 * pas laisse un trou que rien ne redimensionne.
 *
 * Étalon = gtk3 (GtkPaned).
 * ⚠️ EXACTEMENT deux enfants ; un troisième est refusé AVEC un message.
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
#include "widget_paned.h"
#include <FL/Fl.H>
#include <FL/Fl_Tile.H>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_paned_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    bool vertical = false;
    if (attr) {
        const char *v = get_tag_attribute(attr, "orientation");
        if (v && !strcasecmp(v, "vertical")) vertical = true;
    }

    stackelement s = pop();
    Fl_Widget *a = NULL, *b = NULL;
    int retenus = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        Fl_Widget *c = (Fl_Widget *) s.widgets[n];
        if (!c) continue;
        if      (retenus == 0) a = c;
        else if (retenus == 1) b = c;
        else {
            fprintf(stderr, "fltk1sermo: <paned> prend EXACTEMENT deux enfants : "
                            "le %de est ignoré. Emballer le surplus dans une <vbox>.\n",
                    retenus + 1);
            retenus++;
            continue;
        }
        retenus++;
    }
    if (retenus < 2)
        fprintf(stderr, "fltk1sermo: <paned> n'a reçu que %d enfant(s) : la poignée "
                        "n'a rien à partager.\n", retenus);

    /* Surface du pavage : la largeur de travail du port (800), une hauteur
     * tirée des enfants pour ne pas écraser le reste du dialogue. */
    int W = 800;
    int H = 0;
    if (a) H = a->h();
    if (b && b->h() > H) H = b->h();
    if (vertical && a && b) H = a->h() + b->h();
    if (H < 60) H = 60;

    /* Position initiale de la frontière : pixels ou pourcentage. */
    int coupe = vertical ? H / 2 : W / 2;
    if (attr) {
        const char *v = get_tag_attribute(attr, "position");
        if (v && *v) {
            char *fin = NULL;
            double d = g_ascii_strtod(v, &fin);
            if (fin && *fin == '%') {
                if (d > 0 && d < 100) coupe = (int) ((vertical ? H : W) * d / 100.0);
            } else if (d >= 1) {
                coupe = (int) d;
            }
        }
    }
    if (coupe < 10) coupe = 10;
    if (vertical) { if (coupe > H - 10) coupe = H - 10; }
    else          { if (coupe > W - 10) coupe = W - 10; }

    Fl_Tile *tile = new Fl_Tile(0, 0, W, H);
    tile->end();                      /* ne rien capturer d'autre */

    if (a) {
        if (vertical) a->resize(0, 0, W, coupe);
        else          a->resize(0, 0, coupe, H);
        tile->add(a);
    }
    if (b) {
        if (vertical) b->resize(0, coupe, W, H - coupe);
        else          b->resize(coupe, 0, W - coupe, H);
        tile->add(b);
    }

    if (attr) {
        const char *v = get_tag_attribute(attr, "resizable");
        if (v && (!strcasecmp(v, "false") || !strcasecmp(v, "no") || !strcmp(v, "0")))
            tile->deactivate();       /* poignée figée : plus rien ne se saisit */
    }
    return (GtkWidget *) tile;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_paned_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_paned_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_paned_envvar_construct(var->Widget);
}
void widget_paned_clear(variable *var)   { (void) var; }
void widget_paned_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *) var->Widget)->redraw();
}
void widget_paned_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_paned_removeselected(variable *var) { (void) var; }
void widget_paned_save(variable *var)           { (void) var; }
