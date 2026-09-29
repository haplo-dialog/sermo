/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_spinbutton.c — Bouton compteur EFL (elm_spinner)
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
#include "widget_spinbutton.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

GtkWidget *widget_spinbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *sp = elm_spinner_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    {
        const char *dv = attr ? get_tag_attribute(attr, "digits") : NULL;
        int d = dv ? atoi(dv) : 0;
        if (d < 0) d = 0;
        if (d > 15) d = 15;
        evas_object_data_set(sp, "sermo_digits", (void *)(intptr_t) d);
        {   /* le pas suit les décimales, sinon la flèche saute de 1 */
            double pas = 1.0; int i; for (i = 0; i < d; ++i) pas /= 10.0;
            elm_spinner_step_set(sp, pas);
        }
    }

    double vmin = 0, vmax = 100, vstep = 1, vval = 0;
    int decimals = 0;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "min")))        vmin  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "max")))        vmax  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-min")))  vmin  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-max")))  vmax  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "step")))  vstep = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "digits"))) decimals = atoi(v);
    }
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) vval = g_ascii_strtod(def, NULL);
    }

    elm_spinner_min_max_set(sp, vmin, vmax);
    elm_spinner_step_set(sp, vstep);
    elm_spinner_value_set(sp, vval);

    char fmt[32];
    snprintf(fmt, sizeof(fmt), "%%.%df", decimals);
    elm_spinner_label_format_set(sp, fmt);
    evas_object_show(sp);
    /* <input> (ex. echo 30) : valeur initiale par commande */
    if (Attr) {
        gchar *itext = widget_input_text(Attr);
        if (itext) {
            elm_spinner_value_set(sp, g_ascii_strtod(g_strchomp(itext), NULL));
            g_free(itext);
        }
    }
    return (GtkWidget *)sp;
}

gchar *widget_spinbutton_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("0");
    /* Règle de l'étalon : « %.Nf » avec N = digits (0 par défaut). Le port
     * rendait « %g », qui ignore digits. */
    int d = (int)(intptr_t) evas_object_data_get((Evas_Object *)w, "sermo_digits");
    char buf[64], format[8];
    if (d < 0 || d > 15) d = 0;
    snprintf(format, sizeof(format), "%%.%df", d);
    return g_strdup(g_ascii_formatd(buf, sizeof buf, format,
                                    elm_spinner_value_get((Evas_Object *)w)));
}
gchar *widget_spinbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_spinbutton_envvar_construct(var->Widget);
}
void widget_spinbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_spinner_value_set((Evas_Object *)var->Widget, 0.0);
}
void widget_spinbutton_refresh(variable *var) {}
void widget_spinbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_spinbutton_removeselected(variable *var) {}
void widget_spinbutton_save(variable *var) {}
