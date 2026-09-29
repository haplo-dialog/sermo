/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_revealer.cpp — Un enfant qui se montre et se cache (FLTK 1.4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ FLTK montre et cache (show/hide) mais n'anime pas : l'attribut
 * transition= est accepté et IGNORÉ ici. L'état exporté, lui, est identique à
 * l'étalon — dire ce qu'on ne fait pas vaut mieux que de le taire. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_revealer.h"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Grid.H>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

namespace {
/* Le groupe retient son état : l'export le lit, et hide() seul ne se relit pas
 * de façon fiable avant que la fenêtre existe. */
class SermoRevealer : public Fl_Group {
public:
    SermoRevealer(int x, int y, int w, int h) : Fl_Group(x, y, w, h), montre_(false) {}
    void montrer(bool m) { montre_ = m; if (m) show(); else hide(); }
    bool montre() const { return montre_; }
private:
    bool montre_;
};
} /* namespace */

GtkWidget *widget_revealer_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;

    bool montre = false;
    if (attr) {
        const char *v = get_tag_attribute(attr, "reveal");
        if (v && (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || atoi(v) == 1))
            montre = true;
    }
    if (Attr && attributeset_is_avail(Attr, ATTR_DEFAULT)) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (!strcasecmp(def, "true") || !strcasecmp(def, "yes") || atoi(def) == 1))
            montre = true;
    }

    SermoRevealer *grp = new SermoRevealer(0, 0, 800, 60);
    grp->end();
    grp->box(FL_NO_BOX);

    stackelement s = pop();
    int poses = 0, haut = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        Fl_Widget *c = (Fl_Widget *) s.widgets[n];
        if (!c) continue;
        if (poses == 0) { grp->add(c); if (c->h() > haut) haut = c->h(); }
        else fprintf(stderr, "fltk1sermo: <revealer> ne prend QU'UN enfant : le %de "
                             "est ignoré. Emballer le surplus dans une <vbox>.\n",
                     poses + 1);
        poses++;
    }
    if (haut > 0) grp->size(800, haut);
    grp->montrer(montre);
    return (GtkWidget *) grp;
}

/* Export : l'état courant, « true » ou « false » (étalon gtk3). */
gchar *widget_revealer_envvar_construct(GtkWidget *widget)
{
    SermoRevealer *r = dynamic_cast<SermoRevealer *>((Fl_Widget *) widget);
    return g_strdup(r && r->montre() ? "true" : "false");
}
gchar *widget_revealer_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_revealer_envvar_construct(var->Widget);
}
void widget_revealer_clear(variable *var)
{
    SermoRevealer *r = var && var->Widget
        ? dynamic_cast<SermoRevealer *>((Fl_Widget *) var->Widget) : NULL;
    if (r) r->montrer(false);
}
void widget_revealer_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *) var->Widget)->redraw();
}
void widget_revealer_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_revealer_removeselected(variable *var) { (void) var; }
void widget_revealer_save(variable *var)           { (void) var; }
