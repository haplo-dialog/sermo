/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_checkbox.cpp — Case à cocher FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <checkbox> → Fl_Check_Button
 * Export : "true" si coché, "false" sinon.
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
#include "widget_checkbox.h"
#include "jouer_actions.h"

#include <string.h>
#include <stdlib.h>

/* ─── Widgets à bascule : case à cocher, bouton à bascule, bouton radio ──────
 * L'étalon joue leurs actions à chaque changement d'état (« toggled »), avec
 * « if true »/« if false ». Ce port ne les branchait pas sur la case ni sur le
 * radio, et le bouton à bascule les jouait sans ces conditions.
 * Registre borné (maxwidgets vaut 256) : le crochet « activate: » n'agit que sur
 * un bouton qu'il connaît, jamais sur un pointeur d'un autre type.
 * chef = premier radio du groupe du cœur (NULL sinon) ; il garde le radio
 * choisi, pour que celui qui se décoche joue « if false » : FLTK ne rappelle
 * que le bouton cliqué. */
#define FLTK_BASCULES_MAX 512
static struct {
    Fl_Button    *o;
    AttributeSet *jeu;
    Fl_Button    *chef;
    Fl_Button    *choisi;   /* tenu par le chef seulement */
    int           interrupteur;   /* <switch> : signal « changed », libellé ON/OFF */
    int           lu;             /* son <input> a déjà été lu une fois (refresh) */
} bascules[FLTK_BASCULES_MAX];
static int nb_bascules;

static int bascule_index(const Fl_Button *o)
{
    for (int i = 0; i < nb_bascules; i++)
        if (bascules[i].o == o) return i;
    return -1;
}

static void bascule_jouer(int i, int actif)
{
    if (bascules[i].jeu && bascules[i].interrupteur)
        jouer_actions_interrupteur((GtkWidget *)bascules[i].o, bascules[i].jeu, actif);
    else if (bascules[i].jeu)
        jouer_actions_bascule((GtkWidget *)bascules[i].o, bascules[i].jeu, actif);
}

/* Après un changement d'état (clic, ou « activate: ») : joue les actions. */
static void bascule_cb(Fl_Widget *w, void *data)
{
    int i = bascule_index((Fl_Button *)w);
    (void)data;
    if (i < 0) return;
    if (bascules[i].chef) {
        int c = bascule_index(bascules[i].chef);
        Fl_Button *avant = c >= 0 ? bascules[c].choisi : nullptr;
        if (!bascules[i].o->value() || avant == bascules[i].o) return;
        if (c >= 0) bascules[c].choisi = bascules[i].o;
        if (avant) {
            int a = bascule_index(avant);
            if (a >= 0) bascule_jouer(a, 0);   /* le frère décoché d'abord, comme GTK */
        }
        bascule_jouer(i, 1);   /* exit:… peut sortir ici */
        return;
    }
    if (bascules[i].interrupteur)   /* ce que faisait son rappel d'origine */
        bascules[i].o->copy_label(bascules[i].o->value() ? "ON" : "OFF");
    bascule_jouer(i, bascules[i].o->value() ? 1 : 0);   /* exit:… peut sortir ici */
}

static void brancher(GtkWidget *bouton, AttributeSet *Attr, GtkWidget *chef, int interrupteur)
{
    Fl_Button *o = (Fl_Button *)bouton;
    GList *el = NULL;
    int i;

    if (!o || nb_bascules >= FLTK_BASCULES_MAX) return;
    i = nb_bascules++;
    bascules[i].o = o;
    bascules[i].jeu = (Attr && attributeset_get_first(&el, Attr, ATTR_ACTION)) ? Attr : NULL;
    bascules[i].chef = (Fl_Button *)chef;
    bascules[i].choisi = nullptr;
    bascules[i].interrupteur = interrupteur;
    bascules[i].lu = 0;
    if (chef && o->value()) {
        /* Coché d'office par son <default> : le frère qu'il remplace joue ses
         * actions « décoché », comme chez l'étalon où elles sont branchées. */
        int c = bascule_index((Fl_Button *)chef);
        Fl_Button *avant = c >= 0 ? bascules[c].choisi : nullptr;
        if (avant && avant != o) {
            int a = bascule_index(avant);
            if (a >= 0) bascule_jouer(a, 0);
        }
        if (c >= 0) bascules[c].choisi = o;
    }
    o->callback(bascule_cb, nullptr);
}

void fltk1_bascule_brancher(GtkWidget *bouton, AttributeSet *Attr, GtkWidget *chef)
{
    brancher(bouton, Attr, chef, 0);
}

/* L'interrupteur (<switch>) : même chose, au signal « changed » de l'étalon. */
void fltk1_interrupteur_brancher(GtkWidget *bouton, AttributeSet *Attr)
{
    brancher(bouton, Attr, NULL, 1);
}

/* Crochet du contrat : « activate:NOM » bascule comme un clic (le shim fait de
 * gtk_widget_activate() un FALSE). Un radio choisi le reste. */
extern "C" int sermo_be_widget_activate(void *w)
{
    int i = bascule_index((Fl_Button *)w);
    if (i < 0) return 0;
    Fl_Button *b = bascules[i].o;
    if (bascules[i].chef) {
        if (b->value()) return 1;
        b->setonly();
    } else {
        b->value(b->value() ? 0 : 1);
    }
    bascule_cb(b, nullptr);
    return 1;
}

/* <input> d'un widget à bascule (case, interrupteur, bouton radio, bouton à
 * bascule), à chaque « refresh », comme l'étalon gtk3sermo. Au premier passage
 * — le cœur appelle refresh juste après la création — l'état lu se pose sans
 * jouer ses actions (un frère radio décoché joue les siennes), et un <default>
 * l'emporte (l'étalon l'applique après). Aux
 * suivants, un changement d'état bascule comme un clic : les actions jouent.
 * Un bouton radio ne lit que « vrai » : il ne se décoche pas seul, et un
 * <default>false ne l'empêche pas d'être choisi. Jusqu'au 2026-10-13, l'<input>
 * de ces widgets n'était jamais lu. */
void fltk1_bascule_lire(variable *var)
{
    int i = (var && var->Widget) ? bascule_index((Fl_Button *)var->Widget) : -1;
    if (i < 0) return;
    Fl_Button *b = bascules[i].o;
    int  radio   = bascules[i].chef != nullptr;
    int  premier = !bascules[i].lu;
    gint v = widget_input_bool(var->Attributes);

    bascules[i].lu = 1;
    if (v < 0 || (radio && v == 0)) return;
    if (premier) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        if (radio ? (def && (strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
                  : (def != NULL))
            return;
        if (radio) {
            int c = bascule_index(bascules[i].chef);
            Fl_Button *avant = c >= 0 ? bascules[c].choisi : nullptr;
            if (avant && avant != b) {
                int a = bascule_index(avant);
                avant->value(0);
                avant->redraw();
                if (a >= 0) bascule_jouer(a, 0);   /* lui joue « décoché » */
            }
            if (c >= 0) bascules[c].choisi = b;
            fltk1_radio_coche((GtkWidget *)b);   /* widget_radiobutton.cpp */
        }
        b->value(v);
        b->redraw();
        return;
    }
    if (v != (b->value() ? 1 : 0))
        sermo_be_widget_activate(b);
}

/* Crochet du contrat : l'état d'un widget pour condition="active_is_…/
 * sensitive_is_…/visible_is_…(NOM)". « actif » seulement pour un bouton du
 * registre ; sensible et visible = les drapeaux du widget lui-même. */
extern "C" int sermo_be_widget_etat(void *w, int quoi)
{
    Fl_Widget *o = (Fl_Widget *)w;
    if (!o) return -1;
    switch (quoi) {
    case 0: {
        int i = bascule_index((Fl_Button *)w);
        return i >= 0 ? (bascules[i].o->value() ? 1 : 0) : -1;
    }
    case 1: return o->active() ? 1 : 0;
    case 2: return o->visible() ? 1 : 0;
    default: return -1;
    }
}

GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 150, h = 26;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    gchar *label = NULL;
    if (Attr)
        label = attributeset_get_first(&element, Attr, ATTR_LABEL);

    Fl_Check_Button *cb = new Fl_Check_Button(0, 0, w, h, nullptr);
    if (label && *label) cb->copy_label(label);

    /* État initial */
    if (Attr) {
        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && (strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
            cb->value(1);
    }

    fltk1_bascule_brancher((GtkWidget *)cb, Attr, NULL);
    return (GtkWidget *)cb;
}

gchar *widget_checkbox_envvar_construct(GtkWidget *widget)
{
    Fl_Check_Button *cb = (Fl_Check_Button *)widget;
    if (!cb) return g_strdup("false");
    return g_strdup(cb->value() ? "true" : "false");
}

gchar *widget_checkbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_checkbox_envvar_construct(var->Widget);
}

void widget_checkbox_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Check_Button *)var->Widget)->value(0);
}

void widget_checkbox_refresh(variable *var) { fltk1_bascule_lire(var); }

void widget_checkbox_fileselect(variable *var, const char *n, const char *v) {}
void widget_checkbox_removeselected(variable *var) {}
void widget_checkbox_save(variable *var) {}
