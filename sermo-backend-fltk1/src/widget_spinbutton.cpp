/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_spinbutton.cpp — Champ numérique avec boutons +/- FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <spinbutton> → Fl_Spinner
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
#include "widget_spinbutton.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ⛔ atof suit la LOCALE : sous fr_FR, atof("0.5") rend ZÉRO en silence.
 * g_ascii_strtod lit toujours le point décimal. La garde
 * tests/garde_fonctions_interdites.sh l'exige — elle ne regardait pas les
 * .cpp jusqu'au 2026-09-14, d'où ces appels restés en place. */
GtkWidget *widget_spinbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 120, h = 26;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Spinner *sp = new Fl_Spinner(0, 0, w, h, nullptr);
    sp->minimum(0.0);
    sp->maximum(100.0);
    sp->step(1.0);
    sp->value(0.0);
    /* bornes/pas de la forme publique range-min/range-max/range-step */
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "range-min")))  sp->minimum(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "range-max")))  sp->maximum(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "range-step"))) sp->step(g_ascii_strtod(v, NULL));
    }
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) sp->value(g_ascii_strtod(def, NULL));
        /* <input> (ex. echo 30) : valeur initiale par commande */
        {
            gchar *itext = widget_input_text(Attr);
            if (itext) {
                sp->value(g_ascii_strtod(g_strchomp(itext), NULL));
                g_free(itext);
            }
        }
    }

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "min")))   sp->minimum(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "max")))   sp->maximum(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "step")))  sp->step(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "value"))) sp->value(g_ascii_strtod(v, NULL));
        /* digits= : Fl_Spinner range son format et sait le relire — c'est lui
         * qui porte le nombre de décimales jusqu'à l'export. */
        if ((v = get_tag_attribute(attr, "digits"))) {
            static char fmt[8];
            int d = atoi(v); if (d < 0) d = 0; if (d > 15) d = 15;
            snprintf(fmt, sizeof(fmt), "%%.%df", d);
            sp->format(fmt);
        }
    }

    if (Attr) {
        GList *element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) sp->value(g_ascii_strtod(def, NULL));
    }

    return (GtkWidget *)sp;
}

gchar *widget_spinbutton_envvar_construct(GtkWidget *widget)
{
    Fl_Spinner *sp = (Fl_Spinner *)widget;
    if (!sp) return g_strdup("0");
    /* Décimales : celles du format posé à la création (« %.Nf »), 0 par
     * défaut comme l'étalon. Le port rendait « %.6g », qui ignore digits. */
    int d = 0;
    const char *f = sp->format();
    if (f && f[0] == '%' && f[1] == '.') d = atoi(f + 2);
    char buf[64], format[8];
    snprintf(format, sizeof(format), "%%.%df", d);
    return g_strdup(g_ascii_formatd(buf, sizeof buf, format, sp->value()));
}

gchar *widget_spinbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_spinbutton_envvar_construct(var->Widget);
}

void widget_spinbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Spinner *sp = (Fl_Spinner *)var->Widget;
    sp->value(sp->minimum());
}

void widget_spinbutton_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_spinbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_spinbutton_removeselected(variable *var) {}
void widget_spinbutton_save(variable *var) {}
