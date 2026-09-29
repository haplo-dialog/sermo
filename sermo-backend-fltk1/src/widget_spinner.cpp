/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_spinner.cpp — Indicateur de chargement FLTK (Fl_Box animé)
 * sermo — haplo-dialog — GPL-2.0-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_spinner.h"
#include <FL/Fl_Box.H>
#include <FL/Fl.H>
#include <string.h>

static const char * const FRAMES[] = {"⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏"};
static const int NFRAMES = 10;

struct SpinnerBox : Fl_Box {
    int frame;
    SpinnerBox(int X,int Y,int W,int H) : Fl_Box(X,Y,W,H,FRAMES[0]), frame(0) {
        Fl::add_timeout(0.1, _tick, this);
    }
    static void _tick(void *ud) {
        SpinnerBox *s = (SpinnerBox *)ud;
        s->frame = (s->frame+1) % NFRAMES;
        s->copy_label(FRAMES[s->frame]);
        s->redraw();
        Fl::repeat_timeout(0.1, _tick, ud);
    }
    ~SpinnerBox() { Fl::remove_timeout(_tick, this); }
};

GtkWidget *widget_spinner_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    SpinnerBox *sp = new SpinnerBox(0, 0, 40, 40);
    sp->labelfont(FL_HELVETICA_BOLD);
    sp->labelsize(18);
    sp->labelcolor(fl_rgb_color(137,180,250));
    return (GtkWidget *)sp;
}
gchar *widget_spinner_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_spinner_envvar_all_construct(variable *v) { return NULL; }
void widget_spinner_clear(variable *v) {}
void widget_spinner_refresh(variable *v) { if (v && v->Widget) ((Fl_Box *)v->Widget)->redraw(); }
void widget_spinner_fileselect(variable *v, const char*, const char*) {}
void widget_spinner_removeselected(variable *v) {}
void widget_spinner_save(variable *v) {}
