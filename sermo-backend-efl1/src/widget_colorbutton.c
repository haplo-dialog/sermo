/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_colorbutton.c — Sélecteur de couleur EFL (elm_colorselector popup)
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
#include "widget_colorbutton.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct { int r, g, b, a; char hex[8]; } ColorData;

static void _color_changed(void *data, Evas_Object *obj, void *event_info)
{
    ColorData *cd = (ColorData *)data;
    elm_colorselector_color_get(obj, &cd->r, &cd->g, &cd->b, &cd->a);
    snprintf(cd->hex, sizeof(cd->hex), "#%02x%02x%02x", cd->r, cd->g, cd->b);
    /* Mettre à jour la couleur de fond du bouton via style */
    Evas_Object *btn = (Evas_Object *)evas_object_data_get(obj, "button");
    if (btn) {
        /* repeindre la pastille de couleur (contenu du bouton) */
        Evas_Object *sw = (Evas_Object *)evas_object_data_get(btn, "swatch");
        if (sw) evas_object_color_set(sw, cd->r, cd->g, cd->b, 255);
    }
}

static void _btn_clicked(void *data, Evas_Object *obj, void *event_info)
{
    ColorData *cd = (ColorData *)data;
    Evas_Object *parent = evas_object_data_get(obj, "main_win");
    Evas_Object *popup  = elm_popup_add(parent ? parent : obj);
    Evas_Object *cs     = elm_colorselector_add(popup);
    elm_colorselector_color_set(cs, cd->r, cd->g, cd->b, cd->a);
    evas_object_data_set(cs, "button", obj);
    evas_object_smart_callback_add(cs, "changed", _color_changed, cd);
    elm_object_content_set(popup, cs);
    evas_object_show(cs);
    evas_object_show(popup);
}

GtkWidget *widget_colorbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *btn = elm_button_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));

    ColorData *cd = calloc(1, sizeof(ColorData));
    cd->r = 255; cd->g = 255; cd->b = 255; cd->a = 255;
    snprintf(cd->hex, sizeof(cd->hex), "#ffffff");

    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def && def[0] == '#' && strlen(def) == 7) {
            int r, g, b;
            if (sscanf(def+1, "%02x%02x%02x", &r, &g, &b) == 3) {
                cd->r = r; cd->g = g; cd->b = b;
                snprintf(cd->hex, sizeof(cd->hex), "%s", def);
            }
        }
    }

    /* Pastille = rectangle Evas coloré, posé comme contenu (part « icon »)
     * du bouton — parité gtk3/qt6 (aperçu de la couleur, pas un texte hex). */
    {
        Evas_Object *sw = evas_object_rectangle_add(evas_object_evas_get(btn));
        evas_object_color_set(sw, cd->r, cd->g, cd->b, 255);
        evas_object_size_hint_min_set(sw, 48, 20);
        evas_object_show(sw);
        elm_object_content_set(btn, sw);
        evas_object_data_set(btn, "swatch", sw);
    }
    evas_object_data_set(btn, "color_data", cd);
    evas_object_smart_callback_add(btn, "clicked", _btn_clicked, cd);
    evas_object_show(btn);
    return (GtkWidget *)btn;
}

gchar *widget_colorbutton_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("#ffffff");
    ColorData *cd = (ColorData *)evas_object_data_get((Evas_Object *)w, "color_data");
    return g_strdup(cd ? cd->hex : "#ffffff");
}
gchar *widget_colorbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_colorbutton_envvar_construct(var->Widget);
}
void widget_colorbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ColorData *cd = (ColorData *)evas_object_data_get((Evas_Object *)var->Widget, "color_data");
    if (cd) { cd->r=255; cd->g=255; cd->b=255; snprintf(cd->hex,8,"#ffffff"); }
}
/* Couleur « #rrggbb » ou « #rgb » (sans casse). 1 si elle se lit. */
static int couleur_lire(const char *s, int *r, int *g, int *b)
{
    size_t n = s ? strlen(s) : 0;
    if (n < 4 || s[0] != '#' || strspn(s + 1, "0123456789abcdefABCDEF") != n - 1)
        return 0;
    if (n == 7) return sscanf(s + 1, "%02x%02x%02x", r, g, b) == 3;
    if (n == 4) {
        if (sscanf(s + 1, "%1x%1x%1x", r, g, b) != 3) return 0;
        *r *= 17; *g *= 17; *b *= 17;
        return 1;
    }
    return 0;
}

/* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo. Première
 * ligne « couleur|alpha » : la couleur (#rrggbb ou #rgb) repeint la pastille et
 * fait la valeur exportée ; l'alpha (0-65535 chez l'étalon) est gardé pour le
 * sélecteur, l'export de ce port n'en porte pas. Au premier passage, un
 * <default> lisible l'emporte sur la couleur : l'étalon l'applique après.
 * Jusqu'au 2026-10-13 il n'était jamais lu. */
void widget_colorbutton_refresh(variable *var)
{
    ColorData *cd;
    gchar *ligne, **champs;
    gboolean premier;
    int r, g, b;
    unsigned int alpha;

    if (!var || !var->Widget) return;
    cd = (ColorData *)evas_object_data_get((Evas_Object *)var->Widget, "color_data");
    premier = widget_input_premier(var->Widget);
    ligne = widget_input_line(var->Attributes);
    if (!ligne) return;
    if (!cd) { g_free(ligne); return; }
    champs = g_strsplit(ligne, "|", 3);
    if (champs[0]) {
        GList *el = NULL;
        gchar *def = premier ? attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT)
                             : NULL;
        int dr, dg, db;
        int defaut = def && def[0] == '#' && strlen(def) == 7 &&
                     sscanf(def + 1, "%02x%02x%02x", &dr, &dg, &db) == 3;
        if (!defaut && couleur_lire(g_strstrip(champs[0]), &r, &g, &b)) {
            Evas_Object *sw = evas_object_data_get((Evas_Object *)var->Widget, "swatch");
            cd->r = r; cd->g = g; cd->b = b;
            snprintf(cd->hex, sizeof(cd->hex), "#%02x%02x%02x", r, g, b);
            if (sw) evas_object_color_set(sw, r, g, b, 255);
        }
        if (champs[1] && sscanf(champs[1], "%u", &alpha) == 1)
            cd->a = (int)((alpha & 0xffffu) >> 8);
    }
    g_strfreev(champs);
    g_free(ligne);
}
void widget_colorbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_colorbutton_removeselected(variable *var) {}
void widget_colorbutton_save(variable *var) {}
