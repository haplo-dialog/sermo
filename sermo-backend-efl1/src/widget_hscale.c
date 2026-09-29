/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hscale.c — Curseur horizontal EFL (elm_slider)
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
#include "widget_hscale.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

GtkWidget *widget_hscale_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *sl = elm_slider_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    /* digits= : décimales de la valeur EXPORTÉE, comme l'étalon. elm_slider
     * n'a pas de champ pour ça — on le range dans les données de l'objet.
     * ⚠️ Le cœur route <vscale> vers CE créateur (automaton.c) mais son export
     * vers widget_vscale_envvar_construct : la donnée doit donc voyager AVEC
     * l'objet, pas dans une variable de fichier. */
    {
        const char *dv = attr ? get_tag_attribute(attr, "digits") : NULL;
        int d = dv ? atoi(dv) : 0;
        if (d < 0) d = 0;
        if (d > 15) d = 15;
        evas_object_data_set(sl, "sermo_digits", (void *)(intptr_t) d);
    }
    elm_slider_horizontal_set(sl, EINA_TRUE);

    double vmin=0, vmax=100, vstep=1, vval=0;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "min")))  vmin  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "max")))  vmax  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "step"))) vstep = g_ascii_strtod(v, NULL);
    }
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) vval = g_ascii_strtod(def, NULL);
    }
    elm_slider_min_max_set(sl, vmin, vmax);
    elm_slider_step_set(sl, vstep / (vmax - vmin > 0 ? vmax - vmin : 1.0));
    elm_slider_value_set(sl, vval);
    evas_object_show(sl);
    /* <input> (ex. echo 40) : valeur initiale par commande */
    {
        gchar *itext = widget_input_text(Attr);
        if (itext) { elm_slider_value_set(sl, g_ascii_strtod(g_strchomp(itext), NULL)); g_free(itext); }
    }
    return (GtkWidget *)sl;
}

gchar *widget_hscale_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("0");
    /* Règle de l'étalon : « %.Nf » avec N = digits (0 par défaut). Ce port
     * rendait l'entier tronqué. g_ascii_formatd : la locale ne décide pas du
     * séparateur décimal. */
    int d = (int)(intptr_t) evas_object_data_get((Evas_Object *)w, "sermo_digits");
    char buf[64], format[8];
    if (d < 0 || d > 15) d = 0;
    snprintf(format, sizeof(format), "%%.%df", d);
    return g_strdup(g_ascii_formatd(buf, sizeof buf, format,
                                    elm_slider_value_get((Evas_Object *)w)));
}
gchar *widget_hscale_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_hscale_envvar_construct(var->Widget);
}
void widget_hscale_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_slider_value_set((Evas_Object *)var->Widget, 0.0);
}
void widget_hscale_refresh(variable *var) {}
void widget_hscale_fileselect(variable *var, const char *n, const char *v) {}
void widget_hscale_removeselected(variable *var) {}
void widget_hscale_save(variable *var) {}
