/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_image.cpp — Affichage d'image (GtkImage) (portage FLTK)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_image.h"
#include <FL/Fl_Box.H>
#include <FL/Fl_Shared_Image.H>
#include "sermo_icon_theme.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_image_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 120, h = 30;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }
    GList *element = NULL;
    gchar *label = NULL;
    if (Attr) label = attributeset_get_first(&element, Attr, ATTR_LABEL);

    /* <image icon-name="..." icon-size="..."> : icone du theme du bureau,
     * resolue par le coeur (sermo_icon_theme), rendue par FLTK (svg/png) */
    const char *icon = attr ? get_tag_attribute(attr, "icon-name") : NULL;
    int px = sermo_icon_size_px(attr ? get_tag_attribute(attr, "icon-size") : NULL);
    if (attr && get_tag_attribute(attr, "height-request")) px = h;
    Fl_Shared_Image *img = NULL;
    if (icon && *icon) {
        char *path = sermo_icon_lookup(icon, px);
        if (path) { img = Fl_Shared_Image::get(path, px, px); free(path); }
    }
    /* <image file="/chemin" width= height=> : fichier direct */
    const char *file = attr ? get_tag_attribute(attr, "file") : NULL;
    if (!img && file && *file) {
        const char *wv = get_tag_attribute(attr, "width"), *hv = get_tag_attribute(attr, "height");
        int fw = wv ? atoi(wv) : 0, fh = hv ? atoi(hv) : 0;
        img = (fw > 0 && fh > 0) ? Fl_Shared_Image::get(file, fw, fh) : Fl_Shared_Image::get(file);
        if (img) px = (fw > 0 ? fw : img->w());
    }
    if (img) { w = px + 4; h = px + 4; }
    Fl_Box *wdg = new Fl_Box(0, 0, w, h);
    if (img) wdg->image(img);
    else if (label && *label) wdg->copy_label(label);
    return (GtkWidget *)wdg;
}

gchar *widget_image_envvar_construct(GtkWidget *widget) { return g_strdup(""); }
gchar *widget_image_envvar_all_construct(variable *var) { return g_strdup(""); }
void widget_image_clear(variable *var) {}
void widget_image_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *)var->Widget)->redraw();
}
void widget_image_fileselect(variable *var, const char *n, const char *v) {}
void widget_image_removeselected(variable *var) {}
void widget_image_save(variable *var) {}
