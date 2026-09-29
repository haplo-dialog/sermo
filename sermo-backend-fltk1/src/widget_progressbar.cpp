/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_progressbar.cpp — Barre de progression FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <progressbar> → Fl_Progress
 * Export : rien, comme l'étalon gtk3 (un afficheur, pas une saisie)
 * Attributs : value (0-100), text (label affiché dans la barre)
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
#include "widget_progressbar.h"
#include "sermo_progress.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* La commande <input> est lue AU FIL DE L'EAU par le cœur
 * (sermo_progress.h). Jusqu'à la 2.6.8, ce port la lisait EN ENTIER avant
 * d'ouvrir la fenêtre (3,5 s d'écran vide sur examples/progressbar), puis
 * posait un seul nombre : la barre ne progressait jamais et l'action prévue
 * à 100 ne partait pas. */
class SermoProgress : public Fl_Progress {
public:
    SermoProgress(int x, int y, int w, int h) : Fl_Progress(x, y, w, h, nullptr) {}
    ~SermoProgress() override;
    sermo_progress *lecture = nullptr;
};

static void fltk_barre_releve(void *data)
{
    SermoProgress *pb = (SermoProgress *)data;
    if (!pb->lecture) return;
    if (sermo_progress_poll(pb->lecture))
        Fl::repeat_timeout(0.04, fltk_barre_releve, data);
    else
        pb->lecture = nullptr;   /* rendue par le cœur */
}

SermoProgress::~SermoProgress()
{
    Fl::remove_timeout(fltk_barre_releve, this);
    sermo_progress_free(lecture);
}

static void fltk_barre_fraction(void *widget, double fraction)
{
    Fl_Progress *pb = (Fl_Progress *)widget;
    pb->value((float)(fraction * 100.0));
    pb->redraw();
}

static void fltk_barre_texte(void *widget, const char *texte)
{
    Fl_Progress *pb = (Fl_Progress *)widget;
    pb->copy_label(texte);
    pb->redraw();
}

/* ⛔ atof suit la LOCALE : sous fr_FR, atof("0.5") rend ZÉRO en silence.
 * g_ascii_strtod lit toujours le point décimal. La garde
 * tests/garde_fonctions_interdites.sh l'exige — elle ne regardait pas les
 * .cpp jusqu'au 2026-09-14, d'où ces appels restés en place. */
GtkWidget *widget_progressbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 300, h = 24;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    SermoProgress *pb = new SermoProgress(0, 0, w, h);
    pb->minimum(0.0f);
    pb->maximum(100.0f);
    pb->value(0.0f);
    pb->color(FL_BACKGROUND2_COLOR);
    pb->selection_color(FL_SELECTION_COLOR);

    /* Valeur initiale via <value> ou ATTR_DEFAULT */
    if (Attr) {
        /* ⛔ get_tag_attribute() ASSERTE que attr n'est pas NULL, et un
         * <progressbar> sans attribut de balise en a un NUL : le port
         * s'ARRÊTAIT (« ASSERT FAILED: attr != NULL », abort) sur le tag le
         * plus banal qui soit. Mesuré le 2026-09-14. */
        const char *val = attr ? get_tag_attribute(attr, "value") : NULL;
        if (!val) {
            element = NULL;
            val = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        }
        if (val) pb->value((float)g_ascii_strtod(val, NULL));

        /* Texte optionnel affiché dans la barre */
        element = NULL;
        gchar *txt = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (txt && *txt) pb->copy_label(txt);
    }

    pb->lecture = sermo_progress_start(pb, Attr, fltk_barre_fraction, fltk_barre_texte);
    if (pb->lecture)
        Fl::add_timeout(0.0, fltk_barre_releve, pb);
    return (GtkWidget *)pb;
}

gchar *widget_progressbar_envvar_construct(GtkWidget *widget)
{
    /* L'étalon gtk3 n'exporte RIEN pour une barre de progression. */
    (void) widget;
    return g_strdup("");
}

gchar *widget_progressbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_progressbar_envvar_construct(var->Widget);
}

void widget_progressbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Progress *)var->Widget)->value(0.0f);
}

void widget_progressbar_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_progressbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_progressbar_removeselected(variable *var) {}
void widget_progressbar_save(variable *var) {}
