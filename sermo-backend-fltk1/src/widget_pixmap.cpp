/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_pixmap.cpp — Image / icône FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <pixmap> → Fl_Box contenant une Fl_Shared_Image
 *
 * Attributs XML :
 *   file="chemin"      : chemin vers l'image (PNG, JPEG, BMP, XPM…)
 *   width-request / height-request : dimensions demandées
 *
 * Si le fichier n'est pas trouvé ou invalide, le widget reste vide (pas
 * d'erreur fatale).
 *
 * Export : chemin de l'image courante (ou "")
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
#include "widget_pixmap.h"

#include <string.h>
#include <stdlib.h>

struct PixmapData {
    char *filepath;
};

GtkWidget *widget_pixmap_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 64, h = 64;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Box *box = new Fl_Box(0, 0, w, h, nullptr);
    box->box(FL_NO_BOX);

    PixmapData *pd = new PixmapData;
    pd->filepath = NULL;

    /* Chemin de l'image : attribut "file" ou ATTR_DEFAULT */
    const char *filepath = NULL;
    if (attr) filepath = get_tag_attribute(attr, "file");
    if (!filepath && Attr) {
        element = NULL;
        filepath = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
    }

    if (filepath && *filepath) {
        pd->filepath = strdup(filepath);
        Fl_Shared_Image *img = Fl_Shared_Image::get(filepath);
        if (img) {
            /* Redimensionner si nécessaire */
            if (img->w() != w || img->h() != h) {
                Fl_Image *scaled = img->copy(w, h);
                box->image(scaled);
            } else {
                box->image(img);
            }
        }
    }

    box->user_data(pd);
    return (GtkWidget *)box;
}

gchar *widget_pixmap_envvar_construct(GtkWidget *widget)
{
    Fl_Box *box = (Fl_Box *)widget;
    if (!box) return g_strdup("");
    PixmapData *pd = (PixmapData *)box->user_data();
    if (!pd || !pd->filepath) return g_strdup("");
    return g_strdup(pd->filepath);
}

gchar *widget_pixmap_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_pixmap_envvar_construct(var->Widget);
}

void widget_pixmap_clear(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Box *box = (Fl_Box *)var->Widget;
    box->image((Fl_Image *)nullptr);
    PixmapData *pd = (PixmapData *)box->user_data();
    if (pd) { free(pd->filepath); pd->filepath = NULL; }
    box->redraw();
}

void widget_pixmap_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    /* Recharger l'image si le chemin est défini */
    Fl_Box *box = (Fl_Box *)var->Widget;
    PixmapData *pd = (PixmapData *)box->user_data();
    if (pd && pd->filepath) {
        Fl_Shared_Image *img = Fl_Shared_Image::get(pd->filepath);
        if (img) box->image(img);
    }
    box->redraw();
}

void widget_pixmap_fileselect(variable *var, const char *n, const char *v)
{
    /* Permettre la mise à jour du chemin de l'image */
    if (!var || !var->Widget || !v) return;
    Fl_Box *box = (Fl_Box *)var->Widget;
    PixmapData *pd = (PixmapData *)box->user_data();
    if (!pd) return;
    free(pd->filepath);
    pd->filepath = strdup(v);
    Fl_Shared_Image *img = Fl_Shared_Image::get(v);
    if (img) box->image(img);
    box->redraw();
}

void widget_pixmap_removeselected(variable *var) {}
void widget_pixmap_save(variable *var) {}
