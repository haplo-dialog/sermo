/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_frame.cpp — Cadre avec label FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <frame> → Fl_Group avec FL_ENGRAVED_FRAME + label en haut à gauche
 * Contient un Fl_Pack vertical pour ses enfants (issus de la pile).
 * Le label du groupe est affiché par FLTK comme titre du cadre.
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
#include "widget_frame.h"
#include "widget_vbox.h"

#include <string.h>
#include <stdlib.h>

/* Fl_Group::resize deplace les enfants PROPORTIONNELLEMENT : agrandi par
 * son vbox (space-expand), le cadre voyait sa zone de texte remonter sur le
 * titre. On garde l'enfant sous le titre, a la taille du cadre. */
class SermoFrame : public Fl_Group {
public:
    SermoFrame(int X, int Y, int W, int H) : Fl_Group(X, Y, W, H, nullptr) {}
    void resize(int X, int Y, int W, int H) override {
        Fl_Widget::resize(X, Y, W, H);
        for (int i = 0; i < children(); i++) {
            Fl_Widget *c = child(i);
            int ch = c->as_group() ? H - 22 : c->h();
            if (c->as_group() || c->h() > H - 22) ch = H - 22;
            c->resize(X + 4, Y + 18, W - 8, ch);
        }
    }
};

GtkWidget *widget_frame_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 300, h = 200;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    gchar *label = NULL;
    if (Attr)
        label = attributeset_get_first(&element, Attr, ATTR_LABEL);
    if ((!label || !*label) && attr) {
        const char *tv = get_tag_attribute(attr, "label");
        if (tv && *tv) label = (gchar *)tv;
    }

    Fl_Group *grp = new SermoFrame(0, 0, w, h);
    grp->box(FL_ENGRAVED_FRAME);
    grp->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
    if (label && *label) grp->copy_label(label);

    /* Récupérer l'enfant depuis la pile et l'insérer dans le groupe */
    stackelement _se_frame = pop();
    Fl_Widget *child = (Fl_Widget *)_se_frame.widgets[0];
    if (_se_frame.nwidgets > 1) {
        /* L'étalon range TOUS les enfants d'un cadre dans une boîte verticale
         * (sermo-backend-gtk3/src/widget_frame.c). Jusqu'à la 2.6.8 ce port ne
         * gardait que le premier : les suivants étaient créés mais jamais
         * affichés. La <vbox> du port les empile, avec ses règles de taille. */
        push(_se_frame);
        child = (Fl_Widget *)widget_vbox_create(NULL, NULL, 0);
    }
    if (child) {
        /* hauteur adaptée au CONTENU (h=200 figé coupait les widgets
         * suivants : deux frames + boutons > fenêtre) */
        int besoin = 0;
        Fl_Group *cg = child->as_group();
        if (cg && child->user_data() == &SERMO_FLEX_TAG && cg->children() > 0) {
            /* flex colonne : somme ; ligne : max */
            Fl_Flex *fx = (Fl_Flex *)child;
            bool vertical = !fx->horizontal();
            for (int i = 0; i < cg->children(); i++) {
                int ch = cg->child(i)->h();
                if (vertical) besoin += ch + fx->gap();
                else if (ch > besoin) besoin = ch;
            }
            besoin += 4;
        } else if (cg && cg->children() > 0) {
            besoin = child->h();   /* autre groupe (onglets…) : sa hauteur */
        } else {
            /* widget simple (progressbar, texte…) : SA hauteur — l'étirer à
             * la hauteur du frame (200 px) donnait une zone blanche géante */
            besoin = child->h();
        }
        /* largeur : celle du contenu s'il en a demande une (< 800 = pas la
         * largeur brute du flex), sinon le defaut */
        if (!(attr && get_tag_attribute(attr, "width-request")) && child->w() > 0 && child->w() < 800)
            w = child->w() + 8;
        if (besoin > 0) {
            h = besoin + 24;
            grp->size(w, h);
        } else grp->size(w, h);
        child->resize(4, 18, w - 8, cg ? h - 22 : child->h());
        grp->add(child);
    }
    grp->end();

    return (GtkWidget *)grp;
}

/* L'étalon exporte le TITRE du cadre (gtk_frame_get_label) ; un cadre sans
 * titre rend une chaîne vide. Ce port rendait TOUJOURS vide — le cas 38 du banc
 * de comportement l'a mesuré. Un conteneur qui n'exporte rien, ça se décrète ;
 * ici l'étalon exporte, donc on exporte. */
gchar *widget_frame_envvar_construct(GtkWidget *widget)
{
    Fl_Widget  *w = (Fl_Widget *)widget;
    const char *l = w ? w->label() : NULL;
    return g_strdup(l ? l : "");
}

gchar *widget_frame_envvar_all_construct(variable *var)
{
    return g_strdup("");
}

void widget_frame_clear(variable *var) {}

void widget_frame_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_frame_fileselect(variable *var, const char *n, const char *v) {}
void widget_frame_removeselected(variable *var) {}
void widget_frame_save(variable *var) {}
