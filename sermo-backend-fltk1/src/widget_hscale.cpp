/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hscale.cpp — Glissière horizontale FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <hscale> → Fl_Value_Slider (horizontal)
 * Attributs : value, min, max, step
 * Export : valeur courante en chaîne décimale
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
#include "widget_hscale.h"
#include "sermo_scale.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ⛔ atof suit la LOCALE : sous fr_FR, atof("0.5") rend ZÉRO en silence.
 * g_ascii_strtod lit toujours le point décimal. La garde
 * tests/garde_fonctions_interdites.sh l'exige — elle ne regardait pas les
 * .cpp jusqu'au 2026-09-14, d'où ces appels restés en place. */
GtkWidget *widget_hscale_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 300, h = 30;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    SermoScale *sl = new SermoScale(0, 0, w, h);
    sl->type(FL_HORIZONTAL);
    sl->minimum(0.0);
    sl->maximum(100.0);
    sl->step(1.0);
    sl->value(0.0);

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "min")))   sl->minimum(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "max")))   sl->maximum(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "step")))  sl->step(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "value"))) sl->value(g_ascii_strtod(v, NULL));
        /* digits= : décimales de la valeur exportée, comme l'étalon. */
        if ((v = get_tag_attribute(attr, "digits"))) sl->decimales(atoi(v));
    }

    /* Valeur initiale alternative via ATTR_DEFAULT */
    if (Attr) {
        GList *element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) sl->value(g_ascii_strtod(def, NULL));
    }

    /* <input> (ex. echo 65) : valeur initiale par commande */
    {
        gchar *itext = widget_input_text(Attr);
        if (itext) { sl->value(g_ascii_strtod(g_strchomp(itext), NULL)); g_free(itext); }
    }
    return (GtkWidget *)sl;
}

gchar *widget_hscale_envvar_construct(GtkWidget *widget)
{
    Fl_Value_Slider *sl = (Fl_Value_Slider *) widget;
    if (!sl) return g_strdup("0");
    /* Règle de l'étalon : « %.Nf » avec N = digits (0 par défaut). Et
     * g_ascii_formatd, pas snprintf : la locale ne décide pas du séparateur. */
    SermoScale *se = dynamic_cast<SermoScale *>(sl);
    int d = se ? se->decimales() : 0;
    char buf[64], format[8];
    snprintf(format, sizeof(format), "%%.%df", d);
    return g_strdup(g_ascii_formatd(buf, sizeof buf, format, sl->value()));
}

gchar *widget_hscale_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_hscale_envvar_construct(var->Widget);
}

void widget_hscale_clear(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Value_Slider *sl = (Fl_Value_Slider *)var->Widget;
    sl->value(sl->minimum());
}

void widget_hscale_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_hscale_fileselect(variable *var, const char *n, const char *v) {}
void widget_hscale_removeselected(variable *var) {}
void widget_hscale_save(variable *var) {}
