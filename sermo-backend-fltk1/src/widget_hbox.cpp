/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hbox.cpp — Conteneur horizontal FLTK (Fl_Pack horizontal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "stack.h"
#include "widget_hbox.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Input_.H>
#include <FL/Fl_Button.H>

#include <stdlib.h>
#include <stdio.h>

/* largeur naturelle d'un widget feuille (image = son image, texte = mesure) */
static int fltk_natural_w(Fl_Widget *g)
{
    const char *txt = NULL;
    Fl_Output *fo = dynamic_cast<Fl_Output *>(g);
    if (fo) txt = fo->value();
    else if (g->label() && *g->label() && !g->as_group()) txt = g->label();
    int gw = 0, gh = 0;
    if (g->image() && !g->as_group()) {
        gw = g->image()->w() + 4;
        if (txt && *txt) {            /* bouton icone + texte */
            int tw = 0; fl_font(g->labelfont(), g->labelsize()); fl_measure(txt, tw, gh, 0);
            gw += tw + 16;
        }
    } else if (txt && *txt) {
        fl_font(fo ? fo->textfont() : g->labelfont(), fo ? fo->textsize() : g->labelsize());
        fl_measure(txt, gw, gh, 0);
        gw += (dynamic_cast<Fl_Button *>(g) ? 24 : 12);
    } else if (g->as_group()) gw = g->w();               /* cadre : sa largeur */
    else gw = g->w() > 200 ? 200 : g->w();
    return gw;
}

/* l'enfant est-il extensible (space-expand, ou saisie/cadre par defaut) ? */
static bool fltk_child_expands(Fl_Widget *c)
{
    if (sermo_widget_expands(c)) return true;
    if (sermo_widget_noexpand(c)) return false;          /* space-expand="false" explicite */
    if (dynamic_cast<Fl_Input_ *>(c) && !dynamic_cast<Fl_Output *>(c)) return true;   /* entry (pas <text>) */
    if (c->as_group()) return true;                       /* frame, scroll, onglets, vbox/hbox */
    return false;
}

GtkWidget *widget_hbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    /* Fl_Flex en ligne : les enfants se partagent la largeur disponible
     * (etiquette + champ, colonnes, rangees de boutons). */
    Fl_Flex *flex = new Fl_Flex(0, 0, 800, 30, Fl_Flex::ROW);
    flex->end();
    int spacing = 4;
    if (attr) {
        const char *v = get_tag_attribute(attr, "spacing");
        if (v) spacing = atoi(v);
    }
    flex->gap(spacing);
    flex->user_data(&SERMO_FLEX_TAG);

    stackelement s = pop();
    int hmax = 0;
    /* gtkdialog empile par pack_end : sans enfant extensible, la rangee
     * est calee a DROITE a sa taille naturelle (etalon gtk3) — un bourrage
     * flexible en tete absorbe l'espace */
    bool any_expand = false;
    for (int n = 0; n < s.nwidgets; n++)
        if (s.widgets[n] && fltk_child_expands((Fl_Widget *)s.widgets[n])) any_expand = true;
    if (!any_expand && s.nwidgets > 0) {
        Fl_Box *filler = new Fl_Box(0, 0, 1, 1);
        filler->box(FL_NO_BOX);
        flex->add(filler);
        for (int n = 0; n < s.nwidgets; n++) {
            Fl_Widget *c = (Fl_Widget *)s.widgets[n];
            if (!c) continue;
            flex->add(c);
            flex->fixed(c, fltk_natural_w(c));
            if (c->h() > hmax) hmax = c->h();
        }
        if (hmax > 0) flex->size(800, hmax);
        return (GtkWidget *)flex;
    }
    for (int n = 0; n < s.nwidgets; n++) {
        Fl_Widget *c = (Fl_Widget *)s.widgets[n];
        if (!c) continue;
        flex->add(c);
        /* une image (Fl_Box porteur d'image) garde sa largeur : sinon le
         * partage egal lui donnait une demi-ligne et poussait le texte */
        if (c->image() && !c->as_group()) flex->fixed(c, c->image()->w() + 4);
        /* un cadre declare space-expand="false" garde sa largeur */
        else if (c->as_group() && c->user_data() != &SERMO_FLEX_TAG && sermo_widget_noexpand(c))
            flex->fixed(c, c->w());
        /* une feuille non extensible (bouton, texte) garde sa largeur
         * naturelle : seuls les extensibles se partagent le reste */
        else if (!c->as_group() && !fltk_child_expands(c))
            flex->fixed(c, fltk_natural_w(c));
        /* une colonne (vbox) sans space-expand garde sa largeur naturelle :
         * le plus large de ses enfants (image = sa largeur, texte = mesure) */
        Fl_Group *cg = c->as_group();
        if (cg && c->user_data() == &SERMO_FLEX_TAG && !sermo_widget_expands(c)) {
            int nat = 0;
            for (int k = 0; k < cg->children(); k++) {
                Fl_Widget *g = cg->child(k);
                int gw = 0, gh = 0;
                const char *txt = NULL;
                Fl_Output *fo = dynamic_cast<Fl_Output *>(g);   /* <text> du port */
                if (fo) txt = fo->value();
                else if (g->label() && *g->label() && !g->as_group()) txt = g->label();
                /* la vbox a deja etire la boite a sa largeur : prendre celle
                 * de l'IMAGE, pas du widget */
                if (g->image() && !g->as_group()) gw = g->image()->w() + 4;
                else if (txt && *txt) {
                    fl_font(fo ? fo->textfont() : g->labelfont(),
                            fo ? fo->textsize() : g->labelsize());
                    gw = 0; gh = 0;
                    fl_measure(txt, gw, gh, 0);
                    gw += 12;
                } else gw = g->w() > 200 ? 200 : g->w();   /* jamais la largeur brute 800 */
                if (gw > nat) nat = gw;
            }
            if (nat > 0) flex->fixed(c, nat + 4);
        }
        if (c->h() > hmax) hmax = c->h();
    }
    if (hmax > 0) flex->size(800, hmax);
    return (GtkWidget *)flex;
}

void widget_hbox_clear(variable *var) {}
gchar *widget_hbox_envvar_construct(GtkWidget *w) { return NULL; }
gchar *widget_hbox_envvar_all_construct(variable *var) { return NULL; }
void widget_hbox_refresh(variable *var) { if (var && var->Widget) ((Fl_Widget *)var->Widget)->redraw(); }
void widget_hbox_fileselect(variable *var, const char *n, const char *v) {}
void widget_hbox_removeselected(variable *var) {}
void widget_hbox_save(variable *var) {}
