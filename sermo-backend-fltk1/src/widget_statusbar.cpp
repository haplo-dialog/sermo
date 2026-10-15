/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_statusbar.cpp — Barre de statut FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <statusbar> → Fl_Output en bas, style barre de statut
 * (FL_FLAT_BOX, fond légèrement enfoncé, texte aligné à gauche)
 * Export : texte affiché
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
#include "widget_statusbar.h"

#include <string.h>
#include <stdlib.h>

/* Fl_Output + repère du premier passage (faux jusqu'au premier refresh).
 * Membre du widget : rien à libérer. */
struct StatusOutput : Fl_Output {
    bool lu;
    StatusOutput(int X, int Y, int W, int H) : Fl_Output(X, Y, W, H, nullptr), lu(false) {}
};

GtkWidget *widget_statusbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 400, h = 22;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Output *sb = new StatusOutput(0, 0, w, h);
    sb->box(FL_FLAT_BOX);
    sb->color(FL_BACKGROUND_COLOR);
    sb->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    /* Texte initial */
    if (Attr) {
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        /* gtk3sermo pousse <label> puis <default> : le texte montré (et la variable)
         * est <default> s'il est là. Ignoré jusqu'à la 2.8.0 : barre vide. */
        GList *el_def = NULL;
        gchar *def = attributeset_get_first(&el_def, Attr, ATTR_DEFAULT);
        if (def && *def) lbl = def;
        if (lbl && *lbl) sb->value(lbl);
    }

    return (GtkWidget *)sb;
}

gchar *widget_statusbar_envvar_construct(GtkWidget *widget)
{
    Fl_Output *sb = (Fl_Output *)widget;
    if (!sb || !sb->value()) return g_strdup("");
    return g_strdup(sb->value());
}

gchar *widget_statusbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_statusbar_envvar_construct(var->Widget);
}

void widget_statusbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Output *)var->Widget)->value("");
}

/* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo. Jusqu'au
 * 2026-10-13 il n'était jamais lu. La première ligne devient le message
 * affiché (et la valeur exportée) ; une ligne vide l'efface. */
void widget_statusbar_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    StatusOutput *so = (StatusOutput *)var->Widget;
    bool premier = !so->lu;
    so->lu = true;
    gchar *ligne = widget_input_line(var->Attributes);
    /* Premier passage (2026-10-13) : l'étalon pose l'<input> PUIS <label> et
     * <default> ; s'il y en a un, il l'emporte (la commande a tourné quand
     * même). La création les a déjà posés : on n'y touche pas. */
    if (ligne && premier && var->Attributes) {
        GList *el = NULL, *el2 = NULL;
        if (attributeset_get_first(&el, var->Attributes, ATTR_LABEL) ||
            attributeset_get_first(&el2, var->Attributes, ATTR_DEFAULT)) {
            g_free(ligne);
            ligne = NULL;
        }
    }
    if (ligne) {
        ((Fl_Output *)var->Widget)->value(ligne);
        g_free(ligne);
    }
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_statusbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_statusbar_removeselected(variable *var) {}
void widget_statusbar_save(variable *var) {}
