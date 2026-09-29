/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_grid.c — Conteneur de mise en page en tableau (EFL/Elementary)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <grid columns="N"> : elm_table, remplissage EN FLOT (ordre du document,
 * retour à la ligne tous les N enfants). Étalon = gtk3 (GtkGrid).
 *
 * ⚠️ elm_table_pack() prend (colonne, rangée), dans CET ordre — l'inverse de
 * l'intuition « ligne d'abord ». Inverser les deux transpose la grille.
 *
 * ⚠️ <grid> n'est pas <table> : <table> est la liste à colonnes des données.
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
#include "widget_grid.h"
#include <stdlib.h>

static int grid_attr_int(tag_attr *attr, const char *nom, int repli)
{
    if (!attr) return repli;
    const char *v = get_tag_attribute(attr, nom);
    if (!v || !*v) return repli;
    int n = atoi(v);
    return (n >= 0) ? n : repli;
}

GtkWidget *widget_grid_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    Evas_Object *parent = efl_main_win_get();
    Evas_Object *racine = parent ? parent : elm_win_add(NULL, "tmp", ELM_WIN_BASIC);

    int columns = grid_attr_int(attr, "columns", 0);
    if (columns <= 0) {
        fprintf(stderr, "efl1sermo: <grid> sans attribut columns= utilisable : "
                        "une seule colonne.\n");
        columns = 1;
    }
    const int ecart_r = grid_attr_int(attr, "row-spacing", 4);
    const int ecart_c = grid_attr_int(attr, "column-spacing", 8);

    /* Un seul pop : le cœur coalesce les enfants (instruction SUM). */
    stackelement s = pop();
    Evas_Object *enfants[MAXWIDGETS];
    int nb = 0;
    for (int n = 0; n < s.nwidgets && nb < MAXWIDGETS; ++n)
        if (s.widgets[n]) enfants[nb++] = (Evas_Object *) s.widgets[n];

    /* ⚠️ POURQUOI PAS elm_table. C'est le conteneur « naturel » pour une
     * grille, et il a été essayé d'abord : il recalcule son propre minimum à
     * partir de ses cellules et IGNORE celui qu'on lui pose, si bien que ses
     * rangées se superposaient — une seule restait visible (mesuré en
     * capture, 2026-09-13). On aligne donc les colonnes à la main, avec les
     * boîtes que ce port sait dimensionner : une rangée = une elm_box
     * horizontale, chaque cellule forcée à la largeur de SA colonne. Le
     * résultat est le même à l'écran : les colonnes sont alignées d'une
     * rangée à l'autre. */
    const int rangees = nb ? (nb + columns - 1) / columns : 0;
    int larg[MAXWIDGETS];
    for (int c = 0; c < columns && c < MAXWIDGETS; ++c) larg[c] = 0;

    evas_smart_objects_calculate(evas_object_evas_get(racine));
    for (int k = 0; k < nb; ++k) {
        int mw = 0, mh = 0;
        evas_object_size_hint_min_get(enfants[k], &mw, &mh);
        /* Un elm_entry rend min = -1 en largeur : « aucune contrainte », pas
         * « zéro ». Sans plancher, sa colonne disparaissait. 120 px est la
         * largeur que l'entry d'Elementary se donne lui-même à la création. */
        if (mw <= 0) mw = 120;
        int c = k % columns;
        if (c < columns && mw > larg[c]) larg[c] = mw;
    }

    Evas_Object *colonne = elm_box_add(racine);
    elm_box_horizontal_set(colonne, EINA_FALSE);
    elm_box_padding_set(colonne, 0, ecart_r);
    elm_box_align_set(colonne, 0.5, 0.0);

    for (int r = 0; r < rangees; ++r) {
        Evas_Object *rangee = elm_box_add(racine);
        elm_box_horizontal_set(rangee, EINA_TRUE);
        elm_box_padding_set(rangee, ecart_c, 0);
        elm_box_align_set(rangee, 0.0, 0.5);

        for (int c = 0; c < columns; ++c) {
            int k = r * columns + c;
            if (k >= nb) break;
            Evas_Object *e = enfants[k];
            int mw = 0, mh = 0;
            evas_object_size_hint_min_get(e, &mw, &mh);
            if (mh <= 0) mh = 27;
            /* Largeur de la COLONNE, pas de l'enfant : c'est ce qui aligne. */
            evas_object_size_hint_min_set(e, larg[c], mh);
            evas_object_size_hint_weight_set(e, EVAS_HINT_EXPAND, 0.0);
            evas_object_size_hint_align_set(e, EVAS_HINT_FILL, 0.5);
            elm_box_pack_end(rangee, e);
            evas_object_show(e);
        }
        evas_object_size_hint_weight_set(rangee, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(rangee, EVAS_HINT_FILL, 0.5);
        elm_box_pack_end(colonne, rangee);
        evas_object_show(rangee);
    }

    evas_object_size_hint_weight_set(colonne, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(colonne, EVAS_HINT_FILL, 0.0);
    evas_object_show(colonne);
    return (GtkWidget *) colonne;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_grid_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_grid_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_grid_envvar_construct(var->Widget);
}
void widget_grid_clear(variable *var)          { (void) var; }
void widget_grid_refresh(variable *var)        { (void) var; }
void widget_grid_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_grid_removeselected(variable *var) { (void) var; }
void widget_grid_save(variable *var)           { (void) var; }
