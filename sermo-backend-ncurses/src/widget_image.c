/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_image.c — <image> ncurses (terminal) : icone de theme (icon-name) ou fichier
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le nom est resolu ici par le coeur (sermo_icon_theme) ; le fichier est
 * decode et envoye en texture au premier rendu (render.cpp, WT_IMAGE_W).
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "ncurses-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_image.h"
#include "sermo_icon_theme.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_image_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *icon = attr ? get_tag_attribute(attr, "icon-name") : NULL;
    int px = sermo_icon_size_px(attr ? get_tag_attribute(attr, "icon-size") : NULL);
    const char *hr = attr ? get_tag_attribute(attr, "height-request") : NULL;
    if (hr && atoi(hr) > 0) px = atoi(hr);
    (void)Type;

    char *path = (icon && *icon) ? sermo_icon_lookup(icon, px) : NULL;
    if (!path && attr) {
        /* <image file="/chemin" width= height=> : fichier direct */
        const char *file = get_tag_attribute(attr, "file");
        if (file && *file) {
            const char *wv = get_tag_attribute(attr, "width");
            path = strdup(file);
            px = (wv && atoi(wv) > 0) ? atoi(wv) : 0;   /* 0 = taille native */
        }
    }
    if (!path && Attr) {
        GList *el = NULL;
        gchar *file = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (!file || !*file) { el = NULL; file = attributeset_get_first(&el, Attr, ATTR_INPUT); }
        if (file && strncasecmp(file, "file:", 5) == 0) file += 5;
        if (file && *file) { path = strdup(file); px = 0; }
    }
    WidgetNode *node = widget_node_new(WT_IMAGE_W, NULL, path ? path : "");
    node->icon_path = path;          /* NULL = rien a afficher */
    /* L'etalon gtk3 exporte le NOM d'icone (gtk_image_get_icon_name), pas le
     * chemin resolu : on garde donc le nom tel qu'il a ete ecrit. */
    if (icon && *icon) node->tooltip = strdup(icon);
    node->icon_px   = px;
    node->state.image.w = px; node->state.image.h = px;
    return (GtkWidget *)node;
}

gchar *widget_image_envvar_construct(GtkWidget *w)
{
    /* Parite gtk3 : le nom d'icone s'il y en a un, chaine vide sinon (une
     * image chargee depuis un fichier n'expose pas son chemin). */
    WidgetNode *n = (WidgetNode *)w;
    return g_strdup(n && n->tooltip ? n->tooltip : "");
}
gchar *widget_image_envvar_all_construct(variable *var)
{ return (var && var->Widget) ? widget_image_envvar_construct(var->Widget) : NULL; }
void widget_image_clear(variable *var) { (void)var; }
void widget_image_refresh(variable *var) { (void)var; }
void widget_image_fileselect(variable *var, const char *n, const char *v) { (void)var; (void)n; (void)v; }
void widget_image_removeselected(variable *var) { (void)var; }
void widget_image_save(variable *var) { (void)var; }
