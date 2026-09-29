/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_filechooser.cpp — Sélecteur de fichier FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <filechooser> → GtkFileChooserButton : sous FLTK, un Fl_Button qui ouvre
 * fl_file_chooser() et affiche le chemin sélectionné comme étiquette.
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
#include "widget_filechooser.h"

#include <FL/Fl_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>
#include "sermo_icon_theme.h"
#include <string.h>
#include <stdlib.h>

/* Libellé d'invite : UNE seule définition, pour la création et pour l'export.
 * Les deux avaient divergé — le bouton affichait « (Aucun) » et l'export
 * écartait « (Choisir un fichier...) » : sans sélection, la variable sortait
 * à « (Aucun) » au lieu d'être vide (vu le 2026-09-17 par le cas 53). */
#define FILECHOOSER_INVITE "(Aucun)"   /* GTK : « (None) » */

static void filechooser_cb(Fl_Widget *w, void *)
{
    const char *f = fl_file_chooser("Choisir un fichier", "*", w->label());
    if (f) w->copy_label(f);
}

GtkWidget *widget_filechooser_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 200, h = 30;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Button *btn = new Fl_Button(0, 0, w, h, FILECHOOSER_INVITE);
    btn->callback(filechooser_cb);
    /* icone du theme comme le bouton GTK (document-open, 16 px) */
    {
        char *path = sermo_icon_lookup("document-open", 16);
        if (path) {
            Fl_Shared_Image *img = Fl_Shared_Image::get(path, 16, 16);
            if (img) { btn->image(img); btn->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_IMAGE_NEXT_TO_TEXT); }
            free(path);
        }
    }

    /* <default> : chemin initial, posé comme sélection (et affiché) */
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) btn->copy_label(def);
    }
    return (GtkWidget *)btn;
}

gchar *widget_filechooser_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("");
    const char *l = ((Fl_Widget *)widget)->label();
    /* Le libellé d'invite n'est PAS une sélection : exporter vide */
    if (!l || strcmp(l, FILECHOOSER_INVITE) == 0) return g_strdup("");
    return g_strdup(l);
}

gchar *widget_filechooser_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return g_strdup("");
    return widget_filechooser_envvar_construct((GtkWidget *)var->Widget);
}

void widget_filechooser_clear(variable *var) {}

void widget_filechooser_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_filechooser_fileselect(variable *var, const char *n, const char *v)
{
    if (var && var->Widget && v)
        ((Fl_Widget *)var->Widget)->copy_label(v);
}

void widget_filechooser_removeselected(variable *var) {}
void widget_filechooser_save(variable *var) {}
