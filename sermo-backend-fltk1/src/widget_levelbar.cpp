/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_levelbar.cpp — Jauge de niveau FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <levelbar> → Fl_Progress (jauge à plage configurable)
 * Attributs : range-min, range-max, mode (info), width-request, height-request
 * Valeur : <input> / <value> / ATTR_DEFAULT ; export : valeur courante.
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
#include "widget_levelbar.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ⛔ atof suit la LOCALE : sous fr_FR, atof("0.5") rend ZÉRO en silence.
 * g_ascii_strtod lit toujours le point décimal. La garde
 * tests/garde_fonctions_interdites.sh l'exige — elle ne regardait pas les
 * .cpp jusqu'au 2026-09-14, d'où ces appels restés en place. */
GtkWidget *widget_levelbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 300, h = 24;
    float  rmin = 0.0f, rmax = 100.0f;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
        /* ⛔ atof suit la locale : sous fr_FR, g_ascii_strtod("0.5", NULL) rend ZÉRO en
         * silence. C'est ce qui faisait ressortir <levelbar><default>0.5 en
         * « 0 » sur ce port (mesuré le 2026-09-14). */
        if ((v = get_tag_attribute(attr, "range-min")))      rmin = (float)g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-max")))      rmax = (float)g_ascii_strtod(v, NULL);
    }

    Fl_Progress *lb = new Fl_Progress(0, 0, w, h, nullptr);
    lb->minimum(rmin);
    lb->maximum(rmax);
    lb->value(rmin);
    lb->color(FL_BACKGROUND2_COLOR);
    lb->selection_color(FL_SELECTION_COLOR);

    if (Attr) {
        const char *val = attr ? get_tag_attribute(attr, "value") : NULL;
        if (!val) {
            element = NULL;
            val = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        }
        if (val) lb->value((float)g_ascii_strtod(val, NULL));

        element = NULL;
        gchar *txt = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (txt && *txt) lb->copy_label(txt);
    }

    /* <input> (ex. echo 65) : valeur initiale par commande */
    {
        gchar *itext = widget_input_text(Attr);
        if (itext) { lb->value((float)g_ascii_strtod(g_strchomp(itext), NULL)); g_free(itext); }
    }
    return (GtkWidget *)lb;
}

gchar *widget_levelbar_envvar_construct(GtkWidget *widget)
{
    Fl_Progress *lb = (Fl_Progress *)widget;
    if (!lb) return g_strdup("0");
    /* ⛔ « %.0f » tronquait à l'entier : <levelbar><default>0.5</default>
     * ressortait « 0 ». L'étalon rend « 0.5 ». Et g_ascii_formatd, pas
     * snprintf : la locale ne doit pas décider du séparateur décimal. */
    char buf[64];
    return g_strdup(g_ascii_formatd(buf, sizeof buf, "%g", (double) lb->value()));
}

gchar *widget_levelbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_levelbar_envvar_construct(var->Widget);
}

void widget_levelbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Progress *lb = (Fl_Progress *)var->Widget;
    lb->value(lb->minimum());
}

void widget_levelbar_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_levelbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_levelbar_removeselected(variable *var) {}
void widget_levelbar_save(variable *var) {}
