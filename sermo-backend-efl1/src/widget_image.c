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
#include <stdio.h>
#include <unistd.h>

/* EFL ne lit pas un SVG en mémoire (elm_image_memfile_set rend 0, mesuré) :
 * l'icône recolorée est écrite une fois dans le cache de la personne
 * (~/.cache/sermo/icones/<nom>-<couleur>.svg), puis lue comme un fichier.
 * Rend ce chemin (à libérer), ou NULL si l'icône n'est pas symbolique. */
static char *icone_teinte_en_cache(const char *path, unsigned rgb)
{
    size_t taille = 0;
    char *svg = sermo_icon_symbolique_teinte(path, rgb, &taille);
    if (!svg) return NULL;
    const char *xdg = getenv("XDG_CACHE_HOME"), *home = getenv("HOME");
    char dossier[1024], fichier[1400];
    if (xdg && *xdg && xdg[0] == '/') snprintf(dossier, sizeof dossier, "%s/sermo/icones", xdg);
    else if (home && *home)           snprintf(dossier, sizeof dossier, "%s/.cache/sermo/icones", home);
    else { free(svg); return NULL; }
    if (g_mkdir_with_parents(dossier, 0700) != 0) { free(svg); return NULL; }
    const char *base = strrchr(path, '/');
    snprintf(fichier, sizeof fichier, "%s/%s-%06x.svg", dossier, base ? base + 1 : path, rgb & 0xffffffu);
    char tmp[1500];
    snprintf(tmp, sizeof tmp, "%s.%d", fichier, (int)getpid());
    FILE *f = fopen(tmp, "wb");
    int ok = f && fwrite(svg, 1, taille, f) == taille;
    if (f && fclose(f) != 0) ok = 0;
    free(svg);
    if (!ok || rename(tmp, fichier) != 0) { unlink(tmp); return NULL; }   /* écriture atomique */
    return strdup(fichier);
}

/* Cree une elm_image de `px` pixels pour l'icone `icon` ; NULL si absente. */
Evas_Object *efl_theme_icon_new(Evas_Object *parent, const char *icon, int px)
{
    if (!icon || !*icon) return NULL;
    char *path = sermo_icon_lookup(icon, px);
    if (!path) return NULL;
    Evas_Object *img = elm_image_add(parent);
    /* Une icône symbolique (gris foncé) est recolorée en clair, comme GTK le
     * fait selon le thème : le thème d'EFL est sombre, et elle y était presque
     * invisible (galerie du site, jusqu'à la 2.8.1). Le SVG recoloré vit aussi
     * longtemps que l'image. */
    char *teinte = icone_teinte_en_cache(path, 0xdddddd);
    elm_image_file_set(img, teinte ? teinte : path, NULL);
    free(teinte);
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
