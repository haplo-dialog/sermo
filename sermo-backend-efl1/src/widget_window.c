/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_window.c — Fenêtre principale EFL
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
#include "widget_window.h"
#include "sermo_icon_theme.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_window_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *title = "efl1sermo";
    const char *tv;
    /* Titre : attribut de balise <window title="..."> d'abord (comme les
     * autres ports), repli sur ATTR_LABEL. */
    if (attr && (tv = get_tag_attribute(attr, "title")) && *tv) {
        title = tv;
    } else if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) title = lbl;
    }

    /* La fenêtre existe déjà (parent de construction des enfants) :
     * on la CONFIGURE — en créer une neuve laisserait les enfants sur
     * l'autre canvas, irrécupérables en EFL. */
    Evas_Object *win = efl_main_win_get();
    elm_win_title_set(win, title);
    /* Icone de fenetre = icone d'appli du port (efl1sermo/efl1dialog) */
    {
        char *ip = sermo_icon_lookup("efl1sermo", 32);
        if (!ip) ip = sermo_icon_lookup("efl1dialog", 32);
        if (ip) {
            Evas_Object *ic = evas_object_image_add(evas_object_evas_get(win));
            evas_object_image_file_set(ic, ip, NULL);
            evas_object_image_size_get(ic, NULL, NULL);
            int iw = 0, ih = 0; evas_object_image_size_get(ic, &iw, &ih);
            if (iw > 0) { evas_object_image_fill_set(ic, 0, 0, iw, ih);
                          elm_win_icon_object_set(win, ic); }
            else evas_object_del(ic);
            free(ip);
        }
    }

    /* Stocker la fenêtre principale pour les widgets enfants */
    evas_object_data_set(win, "main_win", win);

    /* Fond du theme elementary : sans elm_bg la fenetre etait noire pure
     * derriere les widgets (aucun fond dessine). */
    Evas_Object *bg = elm_bg_add(win);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, bg);
    evas_object_show(bg);

    /* Conformant + layout */
    Evas_Object *conform = elm_conformant_add(win);
    evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, conform);
    evas_object_show(conform);

    Evas_Object *box = elm_box_add(win);
    elm_box_horizontal_set(box, EINA_FALSE);
    elm_box_align_set(box, 0.5, 0.0);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
    /* Le contenu va DIRECTEMENT dans le conformant : pas de scroller. Les
     * dimensions demandees (default-width/height, width/height-request) sont
     * un MINIMUM (regle etalon) — la fenetre GRANDIT au contenu quand il
     * deborde, au lieu de le tronquer derriere une barre de defilement (le
     * pied de page « efl1sermo — haplo-dialog » etait masque a 700 px figes). */
    elm_object_content_set(conform, box);
    evas_object_show(box);

    /* Récupérer l'enfant central (vbox/hbox) */
    stackelement s = pop();
    if (s.widgets[0]) {
        Evas_Object *child = (Evas_Object *)s.widgets[0];
        evas_object_size_hint_weight_set(child, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(child, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(box, child);
        evas_object_show(child);
    }

    /* Taille : width/height-request prime ; sinon la fenetre s'adapte au
     * CONTENU (parite etalon — a 400x300 figes, les bancs « clic en bas »
     * cliquaient dans le vide). */
    int w = 400, h = 300, expw = 0, exph = 0;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "default-width")))  { w = atoi(v); expw = 1; }
        if ((v = get_tag_attribute(attr, "width-request")))  { w = atoi(v); expw = 1; }
        if ((v = get_tag_attribute(attr, "default-height"))) { h = atoi(v); exph = 1; }
        if ((v = get_tag_attribute(attr, "height-request"))) { h = atoi(v); exph = 1; }
    }
    {
        int mw = 0, mh = 0;
        /* Les hints min d'Elementary sont calcules paresseusement au rendu :
         * forcer le calcul de TOUT le canvas avant de les lire. */
        evas_smart_objects_calculate(evas_object_evas_get(win));
        evas_object_size_hint_min_get(box, &mw, &mh);
        if (mw <= 0 && s.widgets[0])
            evas_object_size_hint_min_get((Evas_Object *)s.widgets[0], &mw, &mh);
        /* default/request = MINIMUM : sans dimension demandee on prend le
         * min du contenu ; avec, on l'agrandit si le contenu deborde (jamais
         * on ne rapetisse sous la taille demandee). */
        if (mw > 0) w = expw ? (mw > w ? mw : w) : mw;
        if (mh > 0) h = exph ? (mh > h ? mh : h) : mh;
    }
    evas_object_resize(win, w, h);
    evas_object_show(win);

    return (GtkWidget *)win;
}

gchar *widget_window_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_window_envvar_all_construct(variable *var) { return NULL; }
void   widget_window_clear(variable *var) {}
void   widget_window_refresh(variable *var) {}
void   widget_window_fileselect(variable *var, const char *n, const char *v) {}
void   widget_window_removeselected(variable *var) {}
void   widget_window_save(variable *var) {}
