/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_levelbar.c — Barre de niveau EFL (elm_progressbar)
 * sermo — haplo-dialog — GPL-2.0-or-later
 * Export : la valeur ABSOLUE, dans l'intervalle range-min..range-max (0..1 par
 * défaut), comme GtkLevelBar chez l'étalon. */
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
#include "widget_levelbar.h"
#include <Elementary.h>
#include <string.h>
#include <stdio.h>

/* elm_progressbar ne connaît qu'une fraction 0..1 : l'intervalle et la valeur
 * absolue vivent à part (donnée « niveau »), et seule la fraction est affichée.
 * Jusqu'à la 2.7.0, <input> était divisé par range-max (100 par défaut) : un
 * fichier contenant 0.4 donnait 0.004, là où l'étalon rend 0.4. */
typedef struct { double min, max, valeur; } Niveau;

static void niveau_liberer(void *data, Evas *e, Evas_Object *obj, void *info)
{
    (void)e; (void)obj; (void)info;
    g_free(data);
}

static void niveau_poser(Evas_Object *pb, double v)
{
    Niveau *nv = (Niveau *)evas_object_data_get(pb, "niveau");
    if (!nv) return;
    if (v < nv->min) v = nv->min;          /* GtkLevelBar borne aussi */
    if (v > nv->max) v = nv->max;
    nv->valeur = v;
    elm_progressbar_value_set(pb, nv->max > nv->min
                                  ? (v - nv->min) / (nv->max - nv->min) : 0.0);
}

GtkWidget *widget_levelbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *pb = elm_progressbar_add(win);
    Niveau *nv = g_new0(Niveau, 1);
    const char *v;
    (void)Type;

    elm_progressbar_unit_format_set(pb, NULL);  /* pas de texte */
    nv->min = 0.0;
    nv->max = 1.0;
    if (attr && (v = get_tag_attribute(attr, "range-min")))
        nv->min = g_ascii_strtod(v, NULL);
    if (attr && (v = get_tag_attribute(attr, "range-max")))
        nv->max = g_ascii_strtod(v, NULL);
    evas_object_data_set(pb, "niveau", nv);
    evas_object_event_callback_add(pb, EVAS_CALLBACK_DEL, niveau_liberer, nv);
    niveau_poser(pb, nv->min);

    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def)
            niveau_poser(pb, g_ascii_strtod(def, NULL));
    }
    /* <input> (commande ou fichier) : la valeur absolue, lue au début du
     * contenu. Lue ici seulement (widget_levelbar_refresh est vide) : la
     * commande ne s'exécute qu'une fois. */
    {
        gchar *itext = widget_input_text(Attr);
        if (itext) {
            niveau_poser(pb, g_ascii_strtod(itext, NULL));
            g_free(itext);
        }
    }
    evas_object_show(pb);
    return (GtkWidget *)pb;
}
gchar *widget_levelbar_envvar_construct(GtkWidget *w)
{
    /* « %g » comme l'étalon (0.5, pas 0.500), et sans que la locale décide
     * du séparateur décimal. */
    char buf[64];
    Niveau *nv = w ? (Niveau *)evas_object_data_get((Evas_Object *)w, "niveau") : NULL;
    return g_strdup(g_ascii_formatd(buf, sizeof buf, "%g",
                                    nv ? nv->valeur
                                       : elm_progressbar_value_get((Evas_Object *)w)));
}
gchar *widget_levelbar_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_levelbar_envvar_construct(v->Widget) : NULL; }
void widget_levelbar_clear(variable *v)
{ if (v && v->Widget) niveau_poser((Evas_Object *)v->Widget, 0.0); }
void widget_levelbar_refresh(variable *v) {}
void widget_levelbar_fileselect(variable *v, const char *n, const char *val) {}
void widget_levelbar_removeselected(variable *v) {}
void widget_levelbar_save(variable *v) {}
