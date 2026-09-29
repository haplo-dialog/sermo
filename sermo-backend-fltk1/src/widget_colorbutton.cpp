/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_colorbutton.cpp — Sélecteur de couleur FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <colorbutton> → Fl_Button dont la face affiche la couleur courante.
 *                 Un clic ouvre fl_color_chooser().
 * Export : couleur en format hexadécimal "#RRGGBB"
 * Attribut <default> : couleur initiale (format "#RRGGBB" ou "rgb(r,g,b)")
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
#include "widget_colorbutton.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* uchar est défini par FLTK dans <FL/Fl.H> ; on le redéfinit ici si besoin */
#ifndef FL_UCHAR_DEFINED
typedef unsigned char uchar;
#define FL_UCHAR_DEFINED
#endif

/* Structure données utilisateur attachée au bouton */
struct ColorButtonData {
    uchar r, g, b;   /* couleur courante */
};

static void colorbutton_cb(Fl_Widget *w, void *data)
{
    ColorButtonData *cbd = (ColorButtonData *)data;
    uchar r = cbd->r, g = cbd->g, b = cbd->b;
    if (fl_color_chooser("Choisir une couleur", r, g, b)) {
        cbd->r = r; cbd->g = g; cbd->b = b;
        w->color(fl_rgb_color(r, g, b));
        w->redraw();
    }
}

/* Analyser "#RRGGBB" → r,g,b */
static void parse_color_hex(const char *s, uchar *r, uchar *g, uchar *b)
{
    if (!s) return;
    if (*s == '#') s++;
    if (strlen(s) >= 6) {
        unsigned int rr, gg, bb;
        if (sscanf(s, "%02x%02x%02x", &rr, &gg, &bb) == 3) {
            *r = (uchar)rr; *g = (uchar)gg; *b = (uchar)bb;
        }
    }
}

GtkWidget *widget_colorbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 60, h = 30;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    ColorButtonData *cbd = new ColorButtonData;
    cbd->r = 128; cbd->g = 128; cbd->b = 128;  /* gris par défaut */

    /* Couleur initiale */
    if (Attr) {
        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) parse_color_hex(def, &cbd->r, &cbd->g, &cbd->b);
    }

    Fl_Button *btn = new Fl_Button(0, 0, w, h, nullptr);
    btn->color(fl_rgb_color(cbd->r, cbd->g, cbd->b));
    btn->callback(colorbutton_cb, cbd);

    return (GtkWidget *)btn;
}

gchar *widget_colorbutton_envvar_construct(GtkWidget *widget)
{
    Fl_Button *btn = (Fl_Button *)widget;
    if (!btn) return g_strdup("#808080");
    /* Récupérer les données de couleur */
    ColorButtonData *cbd = (ColorButtonData *)btn->user_data();
    if (!cbd) return g_strdup("#808080");
    char buf[8];
    /* Hexadécimal en minuscules : parité de valeur avec l'étalon gtk3 */
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", cbd->r, cbd->g, cbd->b);
    return g_strdup(buf);
}

gchar *widget_colorbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_colorbutton_envvar_construct(var->Widget);
}

void widget_colorbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Button *btn = (Fl_Button *)var->Widget;
    ColorButtonData *cbd = (ColorButtonData *)btn->user_data();
    if (cbd) { cbd->r = 128; cbd->g = 128; cbd->b = 128; }
    btn->color(FL_GRAY);
    btn->redraw();
}

void widget_colorbutton_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_colorbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_colorbutton_removeselected(variable *var) {}
void widget_colorbutton_save(variable *var) {}
