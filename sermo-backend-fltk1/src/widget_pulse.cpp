/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_pulse.cpp — Barre d'activité indéterminée (portage FLTK)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <pulse> : barre SANS valeur — « ça travaille », pas « 42 % ». FLTK n'a pas
 * de mode indéterminé natif (Fl_Progress veut une valeur) : on dessine le
 * bloc qui va et vient, avancé par un Fl::add_timeout. L'ancienne version
 * posait un simple Fl_Box portant le libellé — aucune barre, et l'export
 * rendait une chaîne VIDE là où l'étalon gtk3 rend « pulse » (écart mesuré
 * au banc de comportement, cas 27).
 *
 * Attributs de balise text= / show-text= et <default> : le texte affiché,
 * comme l'étalon gtk3.
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
#include "widget_pulse.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Bloc qui va et vient. La cadence (12 images/s) est assez lente pour ne pas
 * réveiller le processeur pour rien, assez vive pour se lire comme une
 * activité. Le rappel est retiré à la destruction : pas de timeout orphelin. */
namespace {

const double PULSE_PAS = 1.0 / 12.0;

class SermoPulse : public Fl_Box {
public:
    SermoPulse(int x, int y, int w, int h) : Fl_Box(x, y, w, h), phase_(0.0)
    {
        box(FL_FLAT_BOX);
        Fl::add_timeout(PULSE_PAS, tic, this);
    }
    ~SermoPulse() override { Fl::remove_timeout(tic, this); }

    void draw() override
    {
        /* Gouttière */
        fl_color(fl_lighter(FL_BACKGROUND_COLOR));
        fl_rectf(x(), y(), w(), h());
        fl_color(fl_darker(FL_BACKGROUND_COLOR));
        fl_rect(x(), y(), w(), h());

        /* Bloc : aller-retour sur la largeur utile */
        int bw = w() / 4 < 12 ? 12 : w() / 4;
        int course = w() - bw - 4;
        if (course < 0) course = 0;
        double t = fmod(phase_, 2.0);          /* 0 → 2 */
        double f = (t <= 1.0) ? t : 2.0 - t;   /* 0 → 1 → 0 */
        fl_color(FL_SELECTION_COLOR);
        fl_rectf(x() + 2 + (int)(f * course), y() + 2, bw, h() - 4);

        if (label() && *label()) {
            fl_color(FL_FOREGROUND_COLOR);
            fl_font(labelfont(), labelsize());
            fl_draw(label(), x(), y(), w(), h(), FL_ALIGN_CENTER);
        }
    }

private:
    static void tic(void *v)
    {
        SermoPulse *p = static_cast<SermoPulse *>(v);
        p->phase_ += PULSE_PAS;                /* un aller-retour ≈ 2 s */
        p->redraw();
        Fl::repeat_timeout(PULSE_PAS, tic, v);
    }
    double phase_;
};

} /* namespace */

GtkWidget *widget_pulse_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;
    int w = 120, h = 30;
    const char *text = NULL;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
        if ((v = get_tag_attribute(attr, "text")) && *v)     text = v;
    }
    if (!text && Attr) {
        GList *element = NULL;
        gchar *d = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (d && *d) text = d;
        if (!text) {
            element = NULL;
            gchar *l = attributeset_get_first(&element, Attr, ATTR_LABEL);
            if (l && *l) text = l;
        }
    }

    SermoPulse *wdg = new SermoPulse(0, 0, w, h);
    if (text) wdg->copy_label(text);
    return (GtkWidget *) wdg;
}

gchar *widget_pulse_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("pulse");          /* étalon gtk3 : valeur fixe */
}
gchar *widget_pulse_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_pulse_envvar_construct(var->Widget);
}
void widget_pulse_clear(variable *var) { (void) var; }
void widget_pulse_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *) var->Widget)->redraw();
}
void widget_pulse_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_pulse_removeselected(variable *var) { (void) var; }
void widget_pulse_save(variable *var)           { (void) var; }
