/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_image.c — <image> EFL : icone de theme (icon-name) ou fichier
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le nom d'icone est resolu par le coeur (sermo_icon_theme, freedesktop),
 * le fichier (svg/png) est rendu par elm_image via les chargeurs evas.
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
#include "widget_image.h"
#include "sermo_icon_theme.h"
#include <string.h>
#include <stdlib.h>

/* Cree une elm_image de `px` pixels pour l'icone `icon` ; NULL si absente. */
Evas_Object *efl_theme_icon_new(Evas_Object *parent, const char *icon, int px)
{
    if (!icon || !*icon) return NULL;
    char *path = sermo_icon_lookup(icon, px);
    if (!path) return NULL;
    Evas_Object *img = elm_image_add(parent);
    elm_image_file_set(img, path, NULL);
    elm_image_resizable_set(img, EINA_TRUE, EINA_TRUE);
    elm_image_aspect_fixed_set(img, EINA_TRUE);
    evas_object_size_hint_min_set(img, px, px);
    evas_object_size_hint_max_set(img, px, px);
    evas_object_data_set(img, "image_src", path);   /* libere avec l'objet ? non : fuite volontaire minime */
    evas_object_show(img);
    return img;
}

GtkWidget *widget_image_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    const char *icon = attr ? get_tag_attribute(attr, "icon-name") : NULL;
    int px = sermo_icon_size_px(attr ? get_tag_attribute(attr, "icon-size") : NULL);
    const char *hr = attr ? get_tag_attribute(attr, "height-request") : NULL;
    if (hr && atoi(hr) > 0) px = atoi(hr);
    (void)Type;

    Evas_Object *img = efl_theme_icon_new(parent, icon, px);
    if (!img) {
        /* fichier direct : <default>/<input> */
        const char *file = attr ? get_tag_attribute(attr, "file") : NULL;   /* <image file=...> */
        if ((!file || !*file) && Attr) {
            GList *el = NULL;
            file = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
            if (!file || !*file) { el = NULL; file = attributeset_get_first(&el, Attr, ATTR_INPUT); }
            if (file && strncasecmp(file, "file:", 5) == 0) file += 5;
        }
        img = elm_image_add(parent);
        if (file && *file) {
            const char *wv = attr ? get_tag_attribute(attr, "width") : NULL;
            const char *hv = attr ? get_tag_attribute(attr, "height") : NULL;
            elm_image_file_set(img, file, NULL);
            if (wv && hv && atoi(wv) > 0 && atoi(hv) > 0) {
                evas_object_size_hint_min_set(img, atoi(wv), atoi(hv));
                evas_object_size_hint_max_set(img, atoi(wv), atoi(hv));
            }
            evas_object_data_set(img, "image_src", strdup(file));
        } else {
            /* icone absente : place vide de la taille demandee (l'etalon
             * affiche une image cassee) */
            evas_object_size_hint_min_set(img, px, px);
        }
        evas_object_show(img);
    }
    return (GtkWidget *)img;
}

gchar *widget_image_envvar_construct(GtkWidget *w)
{
    const char *f = w ? (const char *)evas_object_data_get((Evas_Object *)w, "image_src") : NULL;
    return g_strdup(f ? f : "");
}
gchar *widget_image_envvar_all_construct(variable *var)
{ return (var && var->Widget) ? widget_image_envvar_construct(var->Widget) : NULL; }
void widget_image_clear(variable *var) { (void)var; }
void widget_image_refresh(variable *var) { (void)var; }
void widget_image_fileselect(variable *var, const char *n, const char *v) { (void)var; (void)n; (void)v; }
void widget_image_removeselected(variable *var) { (void)var; }
void widget_image_save(variable *var) { (void)var; }
