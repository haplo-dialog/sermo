/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_expander.cpp — Zone dépliable FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <expander> → Fl_Button (triangle ▶/▼) + Fl_Group contenu
 *
 * Il n'existe pas de widget Fl_Expander natif dans FLTK.
 * On simule le comportement avec :
 *   - Un Fl_Button "header" qui affiche ▶ Label / ▼ Label
 *   - Un Fl_Group "body" qui contient le widget enfant (dépilé de la pile)
 *   - Un callback qui montre/cache le body et redimensionne le parent
 *
 * L'ensemble est encapsulé dans un Fl_Group parent.
 *
 * Export : "true" si déplié, "false" si replié
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
#include "widget_expander.h"
#include <FL/Fl_Flex.H>
#include <FL/Fl_Window.H>

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#define HEADER_H 24

struct ExpanderData {
    Fl_Group  *body;
    bool       expanded;
    int        body_h;
    /* Le libellé SANS la flèche. Le rappel de bascule réécrivait l'étiquette du
     * bouton avec la seule flèche (« @v  »), ce qui EFFAÇAIT le titre au premier
     * clic : un expander nommé « Détails » devenait anonyme dès qu'on l'ouvrait.
     * Le titre est donc gardé ici pour être réécrit à chaque bascule. */
    char       titre[256];
};

/* Flèche + titre : une seule façon de composer l'en-tête, utilisée à la
 * création comme à chaque bascule. */
static void expander_pose_entete(Fl_Button *btn, const ExpanderData *ed)
{
    char hdr[288];
    /* ⚠️ « @v » N'EST PAS un symbole FLTK : le port l'employait pour l'état
     * ouvert et FLTK ne dessinait donc RIEN. La rotation s'écrit avec le chiffre
     * du pavé numérique — « @2> » pointe vers le bas, « @> » vers la droite. */
    snprintf(hdr, sizeof(hdr), "%s  %s", ed->expanded ? "@2>" : "@>", ed->titre);
    btn->copy_label(hdr);
}

/* Placer l'en-tête et le corps DANS le groupe, explicitement.
 *
 * ⛔ Sans cela, c'est `Fl_Group::resize()` qui décidait : un groupe dont le
 * `resizable` est lui-même étire TOUS ses enfants au prorata. Le bouton d'en-tête
 * grandissait donc avec le groupe (titre flottant au milieu d'une bande haute) et
 * le corps atterrissait n'importe où — par-dessus le widget voisin. On ne laisse
 * plus la géométrie se déduire : on l'écrit. */
static void expander_place_interieur(Fl_Group *grp, Fl_Button *btn, ExpanderData *ed)
{
    if (!grp || !btn || !ed) return;
    btn->resize(grp->x(), grp->y(), grp->w(), HEADER_H);
    ed->body->resize(grp->x(), grp->y() + HEADER_H, grp->w(), ed->body_h);
}

/* Replacer les voisins après un changement de hauteur.
 *
 * ⛔ Le défaut : le rappel de bascule agrandissait le groupe de l'expander et
 * s'arrêtait là. Or ce port épingle chaque enfant d'une boîte à sa hauteur
 * (`Fl_Flex::fixed()`) au moment de la construction — un enfant qui grandit
 * ENSUITE chevauche donc ses voisins au lieu de les pousser. À l'écran :
 * l'étiquette du bas remontait par-dessus le titre et le contenu sortait du
 * cadre. Aucun banc ne le voyait : le banc compare des valeurs, pas l'écran.
 *
 * Le remède remonte la chaîne des boîtes : pour chacune, l'épingle de l'enfant
 * est remise à sa hauteur réelle, la boîte grandit du même delta — sans quoi
 * `layout()` prendrait la place aux voisins au lieu de l'ajouter — puis elle
 * refait sa répartition. La fenêtre suit en dernier. */
static void expander_replace_les_voisins(Fl_Widget *grp, int delta)
{
    if (delta == 0) return;

    Fl_Widget *enfant = grp;
    for (Fl_Group *p = grp->parent(); p; p = p->parent()) {
        if (p->user_data() == &SERMO_FLEX_TAG) {
            Fl_Flex *boite = (Fl_Flex *)p;
            if (!boite->horizontal() && boite->fixed(enfant))
                boite->fixed(enfant, enfant->h());
            boite->size(boite->w(), boite->h() + delta);
            boite->layout();
        }
        enfant = p;
    }

    Fl_Window *fen = grp->window();
    if (fen) {
        fen->size(fen->w(), fen->h() + delta);
        fen->redraw();
    }
}

static void expander_cb(Fl_Widget *w, void *data)
{
    ExpanderData *ed  = (ExpanderData *)data;
    Fl_Button    *btn = (Fl_Button *)w;
    Fl_Group     *grp = w->parent();

    ed->expanded = !ed->expanded;

    if (ed->expanded) ed->body->show();
    else              ed->body->hide();

    int avant = grp ? grp->h() : 0;
    if (grp) {
        grp->size(grp->w(), ed->expanded ? HEADER_H + ed->body_h : HEADER_H);
        expander_place_interieur(grp, btn, ed);
    }
    expander_pose_entete(btn, ed);

    if (grp) {
        expander_replace_les_voisins(grp, grp->h() - avant);
        grp->redraw();
    } else {
        w->redraw();
    }
}


/* Même règle que l'étalon (gtk3 widget_expander.c) : l'état initial vient de
 * l'ATTRIBUT DE BALISE expanded= — « true », « yes » ou 1 — et un expander sans
 * cet attribut est REPLIÉ. Ce port lisait <default> (que l'étalon ignore) et
 * partait ouvert : le cas 40 du banc mesure les deux écarts. */
static bool expander_ouvert_au_depart(tag_attr *attr)
{
    const char *v = attr ? get_tag_attribute(attr, "expanded") : NULL;
    if (!v) return false;
    return (strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 || atoi(v) == 1);
}

GtkWidget *widget_expander_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 300, h_body = 120;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w      = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h_body = atoi(v);
    }

    gchar *label = NULL;
    if (Attr)
        label = attributeset_get_first(&element, Attr, ATTR_LABEL);

    /* Groupe englobant */
    Fl_Group *grp = new Fl_Group(0, 0, w, HEADER_H);

    /* Bouton header — son étiquette est posée plus bas par
     * expander_pose_entete(), une fois l'état initial connu. */
    Fl_Button *btn = new Fl_Button(0, 0, w, HEADER_H, nullptr);
    btn->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    btn->box(FL_FLAT_BOX);

    /* Zone contenu (masquée par défaut) */
    Fl_Group *body = new Fl_Group(0, HEADER_H, w, h_body);
    body->box(FL_DOWN_BOX);

    stackelement _se_exp = pop();
    Fl_Widget *child = (Fl_Widget *)_se_exp.widgets[0];
    if (child) {
        child->resize(2, HEADER_H + 2, w - 4, h_body - 4);
        body->add(child);
    }
    body->end();
    body->hide();

    grp->end();

    ExpanderData *ed = new ExpanderData;
    ed->body     = body;
    ed->expanded = expander_ouvert_au_depart(attr);
    ed->body_h   = h_body;
    snprintf(ed->titre, sizeof(ed->titre), "%s", label ? label : "");
    /* Le groupe ne redistribue rien tout seul : c'est expander_place_interieur()
     * qui écrit la géométrie, à la création comme à chaque bascule. */
    grp->resizable((Fl_Widget *)nullptr);
    if (ed->expanded) {
        body->show();
        grp->size(grp->w(), HEADER_H + h_body);
    }
    expander_place_interieur(grp, btn, ed);
    expander_pose_entete(btn, ed);

    btn->callback(expander_cb, ed);
    grp->user_data(ed);

    return (GtkWidget *)grp;
}

gchar *widget_expander_envvar_construct(GtkWidget *widget)
{
    Fl_Group *grp = (Fl_Group *)widget;
    if (!grp) return g_strdup("false");
    ExpanderData *ed = (ExpanderData *)grp->user_data();
    if (!ed) return g_strdup("false");
    return g_strdup(ed->expanded ? "true" : "false");
}

gchar *widget_expander_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_expander_envvar_construct(var->Widget);
}

void widget_expander_clear(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Group *grp = (Fl_Group *)var->Widget;
    ExpanderData *ed = (ExpanderData *)grp->user_data();
    if (ed && ed->expanded) {
        ed->expanded = false;
        ed->body->hide();
        grp->size(grp->w(), HEADER_H);
        grp->redraw();
    }
}

void widget_expander_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_expander_fileselect(variable *var, const char *n, const char *v) {}
void widget_expander_removeselected(variable *var) {}
void widget_expander_save(variable *var) {}
