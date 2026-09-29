/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_paned.c — Deux zones et une poignée déplaçable (EFL/Elementary)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <paned> : elm_panes, le conteneur d'Elementary fait pour ça. Les contenus
 * s'accrochent aux parts « left » et « right » (ou « top »/« bottom »).
 *
 * ⚠️ PIÈGE DE VOCABULAIRE : elm_panes_horizontal_set(EINA_TRUE) veut dire
 * « poignée HORIZONTALE », donc deux zones l'une AU-DESSUS de l'autre — c'est
 * notre orientation="vertical". Les deux conventions sont inverses ; les
 * confondre fait basculer la fenêtre d'un quart de tour sans rien casser
 * d'autre, donc sans que personne ne s'en aperçoive tout de suite.
 *
 * Étalon = gtk3 (GtkPaned).
 * ⚠️ EXACTEMENT deux enfants ; un troisième est refusé AVEC un message.
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
#include "widget_paned.h"
#include <stdlib.h>

GtkWidget *widget_paned_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    Evas_Object *parent = efl_main_win_get();
    Evas_Object *panes = elm_panes_add(parent ? parent
                                              : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
    int vertical = 0;

    if (attr) {
        const char *v = get_tag_attribute(attr, "orientation");
        if (v && !strcasecmp(v, "vertical")) vertical = 1;
    }
    /* Voir le piège de vocabulaire en tête de fichier. */
    elm_panes_horizontal_set(panes, vertical ? EINA_TRUE : EINA_FALSE);

    stackelement s = pop();
    int retenus = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        Evas_Object *c = (Evas_Object *) s.widgets[n];
        if (!c) continue;
        if (retenus >= 2) {
            fprintf(stderr, "efl1sermo: <paned> prend EXACTEMENT deux enfants : "
                            "le %de est ignoré. Emballer le surplus dans une <vbox>.\n",
                    retenus + 1);
            retenus++;
            continue;
        }
        evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(c, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_object_part_content_set(panes, retenus == 0 ? "left" : "right", c);
        evas_object_show(c);
        retenus++;
    }
    if (retenus < 2)
        fprintf(stderr, "efl1sermo: <paned> n'a reçu que %d enfant(s) : la poignée "
                        "n'a rien à partager.\n", retenus);

    /* Position initiale : elm_panes raisonne en FRACTION (0..1) de la place
     * donnée au contenu gauche/haut. Une valeur en pixels doit donc être
     * rapportée à la taille de la fenêtre — à défaut de mieux à la création,
     * on prend la largeur/hauteur demandée du dialogue quand elle existe. */
    if (attr) {
        const char *v = get_tag_attribute(attr, "position");
        if (v && *v) {
            char *fin = NULL;
            double d = g_ascii_strtod(v, &fin);
            if (fin && *fin == '%') {
                if (d > 0 && d < 100) elm_panes_content_left_size_set(panes, d / 100.0);
            } else if (d >= 1) {
                const char *dim = get_tag_attribute(attr, vertical ? "height-request"
                                                                   : "width-request");
                double total = (dim && atoi(dim) > 0) ? atoi(dim) : (vertical ? 400.0 : 600.0);
                double f = d / total;
                if (f > 0.05 && f < 0.95) elm_panes_content_left_size_set(panes, f);
            }
        }
    }
    if (attr) {
        const char *v = get_tag_attribute(attr, "resizable");
        if (v && (!strcasecmp(v, "false") || !strcasecmp(v, "no") || !strcmp(v, "0")))
            elm_object_disabled_set(panes, EINA_TRUE);   /* poignée figée */
    }

    evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(panes, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(panes);
    return (GtkWidget *) panes;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_paned_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_paned_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_paned_envvar_construct(var->Widget);
}
void widget_paned_clear(variable *var)          { (void) var; }
void widget_paned_refresh(variable *var)        { (void) var; }
void widget_paned_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_paned_removeselected(variable *var) { (void) var; }
void widget_paned_save(variable *var)           { (void) var; }
