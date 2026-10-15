/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_checkbox.c — Case à cocher EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
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
 * « if true »/« if false ». Ce port ne les gardait pas : aucune ne jouait.
 * Registre borné (maxwidgets vaut 256) : le crochet « activate: » n'agit que sur
 * un objet qu'il connaît, jamais sur un pointeur d'un autre type.
 * chef = premier bouton du groupe radio (NULL pour une case) ; le chef garde le
 * bouton choisi, pour que celui qui se décoche joue « if false ». */
#define EFL_BASCULES_MAX 512
static struct {
    Evas_Object  *o;
    AttributeSet *jeu;
    Evas_Object  *chef;
    Evas_Object  *choisi;   /* tenu par le chef seulement */
    int           interrupteur;   /* <switch> : signal « changed » */
    int           lu;             /* son <input> a déjà été lu une fois (refresh) */
} bascules[EFL_BASCULES_MAX];
static int nb_bascules;

static int bascule_index(Evas_Object *o)
{
    for (int i = 0; i < nb_bascules; i++)
        if (bascules[i].o == o) return i;
    return -1;
}

static int bascule_active(int i)
{
    Evas_Object *o = bascules[i].o;
    if (bascules[i].chef)
        return elm_radio_value_get(o) == elm_radio_state_value_get(o);
    return elm_check_state_get(o) ? 1 : 0;
}

static void bascule_jouer(int i, int actif)
{
    if (bascules[i].jeu && bascules[i].interrupteur)
        jouer_actions_interrupteur((GtkWidget *)bascules[i].o, bascules[i].jeu, actif);
    else if (bascules[i].jeu)
        jouer_actions_bascule((GtkWidget *)bascules[i].o, bascules[i].jeu, actif);
}

/* Après un changement d'état (clic, ou « activate: ») : joue les actions. */
static void bascule_change(void *data, Evas_Object *obj, void *ev)
{
    int i = bascule_index(obj);
    (void)data; (void)ev;
    if (i < 0) return;
    if (bascules[i].chef) {
        int c = bascule_index(bascules[i].chef);
        Evas_Object *avant = c >= 0 ? bascules[c].choisi : NULL;
        if (!bascule_active(i) || avant == obj) return;
        if (c >= 0) bascules[c].choisi = obj;
        if (avant) {
            int a = bascule_index(avant);
            if (a >= 0) bascule_jouer(a, 0);   /* le frère décoché d'abord, comme GTK */
        }
        bascule_jouer(i, 1);   /* exit:… peut sortir ici */
        return;
    }
    bascule_jouer(i, bascule_active(i));   /* exit:… peut sortir ici */
}

static void brancher(Evas_Object *o, AttributeSet *Attr, Evas_Object *chef, int interrupteur)
{
    GList *el = NULL;
    int i;

    if (!o || nb_bascules >= EFL_BASCULES_MAX) return;
    i = nb_bascules++;
    bascules[i].o = o;
    bascules[i].jeu = (Attr && attributeset_get_first(&el, Attr, ATTR_ACTION)) ? Attr : NULL;
    bascules[i].chef = chef;
    bascules[i].choisi = NULL;
    bascules[i].interrupteur = interrupteur;
    bascules[i].lu = 0;
    if (chef && bascule_active(i)) {
        /* Coché d'office par son <default> : le frère qu'il remplace joue ses
         * actions « décoché », comme chez l'étalon où elles sont branchées. */
        int c = bascule_index(chef);
        Evas_Object *avant = c >= 0 ? bascules[c].choisi : NULL;
        if (avant && avant != o) {
            int a = bascule_index(avant);
            if (a >= 0) bascule_jouer(a, 0);
        }
        if (c >= 0) bascules[c].choisi = o;
    }
    evas_object_smart_callback_add(o, "changed", bascule_change, NULL);
}

void efl_bascule_brancher(Evas_Object *o, AttributeSet *Attr, Evas_Object *chef)
{
    brancher(o, Attr, chef, 0);
}

/* L'interrupteur (<switch>) : même chose, au signal « changed » de l'étalon. */
void efl_interrupteur_brancher(Evas_Object *o, AttributeSet *Attr)
{
    brancher(o, Attr, NULL, 1);
}

/* Crochet du contrat : « activate:NOM » bascule comme un clic (le shim fait de
 * gtk_widget_activate() un FALSE). Un bouton radio choisi le reste. */
int sermo_be_widget_activate(void *w)
{
    int i = bascule_index((Evas_Object *)w);
    if (i < 0) return 0;
    if (bascules[i].chef) {
        if (bascule_active(i)) return 1;
        elm_radio_value_set(bascules[i].o, elm_radio_state_value_get(bascules[i].o));
    } else {
        elm_check_state_set(bascules[i].o, !elm_check_state_get(bascules[i].o));
    }
    bascule_change(NULL, bascules[i].o, NULL);
    return 1;
}

/* <input> d'un widget à bascule (case, interrupteur, bouton radio, bouton à
 * bascule), à chaque « refresh », comme l'étalon gtk3sermo. Au premier passage
 * — le cœur appelle refresh juste après la création — l'état lu se pose sans
 * jouer ses actions (poser l'état par programme n'émet pas « changed » ; un
 * frère radio décoché joue les siennes), et un
 * <default> l'emporte (l'étalon l'applique après). Aux suivants, un changement
 * d'état bascule comme un clic : les actions jouent. Un bouton radio ne lit que
 * « vrai » : il ne se décoche pas seul, et un <default>false ne l'empêche pas
 * d'être choisi. Jusqu'au 2026-10-13, l'<input> de ces widgets n'était jamais lu. */
void efl_bascule_lire(variable *var)
{
    int i = (var && var->Widget) ? bascule_index((Evas_Object *)var->Widget) : -1;
    int radio, premier;
    gint v;

    if (i < 0) return;
    radio   = bascules[i].chef != NULL;
    premier = !bascules[i].lu;
    v = widget_input_bool(var->Attributes);
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
            Evas_Object *avant = c >= 0 ? bascules[c].choisi : NULL;
            elm_radio_value_set(bascules[i].o, elm_radio_state_value_get(bascules[i].o));
            if (c >= 0) bascules[c].choisi = bascules[i].o;
            if (avant && avant != bascules[i].o) {
                int a = bascule_index(avant);
                if (a >= 0) bascule_jouer(a, 0);   /* lui joue « décoché » */
            }
        } else {
            elm_check_state_set(bascules[i].o, v);
        }
        return;
    }
    if (v != bascule_active(i))
        sermo_be_widget_activate(bascules[i].o);
}

/* Crochet du contrat : l'état d'un widget pour condition="active_is_…/
 * sensitive_is_…/visible_is_…(NOM)". « actif » seulement pour un widget du
 * registre ; sensible et visible = ceux de l'objet lui-même. */
int sermo_be_widget_etat(void *w, int quoi)
{
    Evas_Object *o = (Evas_Object *)w;
    int i;
    if (!o) return -1;
    switch (quoi) {
    case 0:
        i = bascule_index(o);
        return i >= 0 ? bascule_active(i) : -1;
    case 1: return elm_object_disabled_get(o) ? 0 : 1;
    case 2: return evas_object_visible_get(o) ? 1 : 0;
    default: return -1;
    }
}

GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *chk = elm_check_add(parent);

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) elm_object_text_set(chk, lbl);
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def) {
            int v = (strcasecmp(def,"true")==0 || strcmp(def,"1")==0);
            elm_check_state_set(chk, v);
        }
    }
    efl_bascule_brancher(chk, Attr, NULL);
    evas_object_show(chk);
    return (GtkWidget *)chk;
}

gchar *widget_checkbox_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("false");
    return g_strdup(elm_check_state_get((Evas_Object *)widget) ? "true" : "false");
}

gchar *widget_checkbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_checkbox_envvar_construct(var->Widget);
}

void widget_checkbox_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_check_state_set((Evas_Object *)var->Widget, 0);
}
void widget_checkbox_refresh(variable *var) { efl_bascule_lire(var); }
void widget_checkbox_fileselect(variable *var, const char *n, const char *v) {}
void widget_checkbox_removeselected(variable *var) {}
void widget_checkbox_save(variable *var) {}
