/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_vbox.c — Conteneur vertical EFL (elm_box vertical)
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
#include "stack.h"
#include "widget_vbox.h"
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------- */
/* Rangement d'une boîte (colonne ou rangée) — règle de l'étalon gtk3     */
/* (sermo-backend-gtk3/src/widget_vbox.c et widget_hbox.c), partagée par  */
/* widget_hbox.c.                                                         */
/*                                                                        */
/* - valeur courante (étirement, remplissage) = celle de la boîte, sinon  */
/*   « non dit » (project_space_*) ;                                      */
/* - parcours des enfants (colonne : ordre du document ; rangée : à       */
/*   l'envers, comme les gtk_box_pack_end de l'étalon) ; un enfant qui a  */
/*   son propre space-expand / space-fill REMPLACE la valeur courante,    */
/*   pour lui et pour les suivants du parcours (jamais remise à zéro) ;   */
/* - « non dit » : défaut du type (colonne : edit, frame, fenêtre         */
/*   défilante, paned ; rangée : edit, frame, fenêtre défilante, entry) ; */
/* - pas de remontée d'étirement d'un enfant vers sa boîte : en GTK 3    */
/*   elle ne passe que par hexpand/vexpand, que sermo ne pose jamais      */
/*   (mesuré : system-tools, cadre « Actions rapides ») ;                */
/* - homogeneous="true" : cases égales sur TOUTE la longueur de la boîte, */
/*   un enfant sans remplissage est centré dans sa case ;                 */
/* - rangée : sans enfant étiré, le contenu est calé à DROITE.            */
/* ------------------------------------------------------------------- */

static int vrai_ou_faux(const char *v)
{
    if (!v) return -1;
    return (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || atoi(v) == 1) ? 1 : 0;
}

/* Ce qui a le droit d'occuper toute la hauteur qu'on lui donne. Une barre,
 * un curseur, un champ, un texte gardent leur hauteur propre (centrés). */
static int remplit_en_hauteur(int type, Evas_Object *c)
{
    switch (type) {
        case WIDGET_BUTTON: case WIDGET_OKBUTTON: case WIDGET_CANCELBUTTON:
        case WIDGET_HELPBUTTON: case WIDGET_NOBUTTON: case WIDGET_YESBUTTON:
        case WIDGET_TOGGLEBUTTON: case WIDGET_COLORBUTTON: case WIDGET_FONTBUTTON:
        case WIDGET_FILECHOOSER: case WIDGET_MENUBUTTON:
        case WIDGET_VBOX: case WIDGET_HBOX: case WIDGET_FRAME: case WIDGET_ASPECTFRAME:
        case WIDGET_NOTEBOOK: case WIDGET_SCROLLEDW: case WIDGET_PANED:
        case WIDGET_EXPANDER: case WIDGET_EVENTBOX: case WIDGET_DRAWINGAREA:
        case WIDGET_GRID: case WIDGET_STACK: case WIDGET_OVERLAY: case WIDGET_FLOWBOX:
        case WIDGET_VSEPARATOR: case WIDGET_VSCALE:
            return 1;
        default:
            break;
    }
    return evas_object_data_get(c, "sw_rect") != NULL;
}

/* Une boîte d'Elementary hérite du PLUS PETIT maximum en travers de ses
 * enfants : une image bornée à 48 px bornait toute la colonne à 48 px, et
 * le libellé voisin débordait (t1 : « DossierDossier… »). GTK n'a pas cette
 * borne : la colonne prend la largeur reçue et l'image y est centrée. On
 * glisse donc un tel enfant dans une boîte Evas brute, qui ne publie qu'un
 * minimum (jamais de maximum) et centre l'enfant à sa taille. */
static Evas_Object *sans_borne_en_travers(Evas_Object *c, int rangee)
{
    Evas_Coord mw = -1, mh = -1;
    evas_object_size_hint_max_get(c, &mw, &mh);
    if ((rangee ? mh : mw) < 0) return c;
    Evas_Object *env = evas_object_box_add(evas_object_evas_get(c));
    evas_object_box_layout_set(env, evas_object_box_layout_horizontal, NULL, NULL);
    evas_object_box_align_set(env, 0.5, 0.5);
    evas_object_size_hint_align_set(c, 0.5, 0.5);
    evas_object_size_hint_weight_set(c, 0.0, 0.0);
    evas_object_box_append(env, c);
    evas_object_show(c);
    return env;
}

void efl_boite_ranger(Evas_Object *box, stackelement *s, int rangee, tag_attr *attr)
{
    int exp_c = project_space_expand, fill_c = project_space_fill;
    int homog = 0, n, i;
    int exp[MAXWIDGETS], fill[MAXWIDGETS];
    int un_etire = 0;

    if (attr) {
        int v;
        if ((v = vrai_ou_faux(get_tag_attribute(attr, "space-expand"))) != -1) exp_c = v;
        if ((v = vrai_ou_faux(get_tag_attribute(attr, "space-fill"))) != -1) fill_c = v;
        homog = vrai_ou_faux(get_tag_attribute(attr, "homogeneous")) == 1;
    }
    elm_box_homogeneous_set(box, homog ? EINA_TRUE : EINA_FALSE);

    /* 1. valeurs (étirement, remplissage) de chaque enfant, dans l'ordre du
     *    parcours de l'étalon */
    for (i = 0; i < s->nwidgets; ++i) {
        n = rangee ? s->nwidgets - 1 - i : i;
        Evas_Object *c = (Evas_Object *)s->widgets[n];
        int t = s->widgettypes[n], defaut;
        if (evas_object_data_get(c, "sermo_expand"))   exp_c = 1;
        if (evas_object_data_get(c, "sermo_noexpand")) exp_c = 0;
        if (evas_object_data_get(c, "sermo_fill"))     fill_c = 1;
        if (evas_object_data_get(c, "sermo_nofill"))   fill_c = 0;
        defaut = (t == WIDGET_EDIT || t == WIDGET_FRAME || t == WIDGET_SCROLLEDW ||
                  (rangee ? t == WIDGET_ENTRY : t == WIDGET_PANED));
        exp[n]  = exp_c  != -1 ? exp_c  : defaut;
        fill[n] = fill_c != -1 ? fill_c : defaut;
    }

    /* 2. rangement dans l'ordre du document */
    for (n = 0; n < s->nwidgets; ++n) {
        Evas_Object *c = (Evas_Object *)s->widgets[n];
        int haut = remplit_en_hauteur(s->widgettypes[n], c);
        Evas_Object *env = sans_borne_en_travers(c, rangee);
        int etire = homog || exp[n];
        double poids = etire ? EVAS_HINT_EXPAND : 0.0;

        if (exp[n]) un_etire = 1;
        if (rangee) {
            /* axe de la rangée : la case reçue, remplie ou centrée ;
             * en travers : toute la hauteur pour ce qui peut la prendre */
            if (env != c) haut = 1;   /* l'enveloppe prend la hauteur, l'enfant reste centré */
            c = env;
            evas_object_size_hint_weight_set(c, poids, haut ? EVAS_HINT_EXPAND : 0.0);
            evas_object_size_hint_align_set(c, fill[n] ? EVAS_HINT_FILL : 0.5,
                                            haut ? EVAS_HINT_FILL : 0.5);
        } else {
            c = env;
            evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, poids);
            evas_object_size_hint_align_set(c, EVAS_HINT_FILL,
                                            (fill[n] && haut) ? EVAS_HINT_FILL : 0.5);
        }
        elm_box_pack_end(box, c);
        evas_object_show(c);
    }

    /* rangée sans enfant étiré : gtk_box_pack_end cale tout à DROITE */
    if (rangee)
        elm_box_align_set(box, (un_etire || homog) ? 0.0 : 1.0, 0.5);
}

GtkWidget *widget_vbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *box = elm_box_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    elm_box_horizontal_set(box, EINA_FALSE);
    elm_box_align_set(box, 0.5, 0.0);   /* contenu ancre en HAUT (etalon), pas centre */

    if (attr) {
        const char *v = get_tag_attribute(attr, "spacing");
        if (v) elm_box_padding_set(box, 0, atoi(v));
    }

    /* Le conteneur récupère UN seul élément de pile contenant tous ses
     * enfants (coalescés par l'instruction SUM de l'automate), exactement
     * comme le port GTK de référence. Un pop() par enfant provoquerait un
     * « stack underflow » faute de sentinelle. widgets[0] est le premier
     * enfant déclaré ; pack_end conserve donc l'ordre du document. */
    stackelement s = pop();
    efl_boite_ranger(box, &s, 0, attr);
    evas_object_show(box);
    return (GtkWidget *)box;
}

gchar *widget_vbox_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_vbox_envvar_all_construct(variable *var) { return NULL; }
void   widget_vbox_clear(variable *var) {}
void   widget_vbox_refresh(variable *var) {}
void   widget_vbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_vbox_removeselected(variable *var) {}
void   widget_vbox_save(variable *var) {}
