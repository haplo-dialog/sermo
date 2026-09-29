/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hscale.cpp — Glissière horizontale Qt6 (QSlider)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <hscale> → QSlider(Qt::Horizontal)
 * Attributs : value, min, max, step
 * Export : valeur courante en chaîne décimale
 *
 * Note : QSlider travaille en entiers ; on multiplie par 1000 si step < 1.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_hscale.h"
#include <cmath>
#include <QtCore/QString>
#include <QtCore/QByteArray>

#include <QtWidgets/QSlider>
#include <QtCore/Qt>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_hscale_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    double vmin = 0.0, vmax = 100.0, vstep = 1.0, vval = 0.0;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "range-min")) || (v = get_tag_attribute(attr, "min")))   vmin  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-max")) || (v = get_tag_attribute(attr, "max")))   vmax  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-step")) || (v = get_tag_attribute(attr, "step"))) vstep = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-value")) || (v = get_tag_attribute(attr, "value"))) vval  = g_ascii_strtod(v, NULL);
    }
    if (Attr) {
        GList *element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) vval = g_ascii_strtod(def, NULL);
    }
    vval = widget_command_value(Attr, vval);   /* <input>echo N</input> prioritaire */

    /* ⚠️ QSlider ne connaît que des ENTIERS. Ce port posait donc
     * setValue((int)vval) : <hscale digits="2"><default>2.5</default>
     * devenait 2, et l'attribut digits= n'avait aucun effet — ni à l'écran,
     * ni à l'export. On travaille maintenant sur un axe MIS À L'ÉCHELLE de
     * 10^digits ; la valeur réelle se retrouve en divisant. */
    int digits = 0;
    if (attr) { const char *v = get_tag_attribute(attr, "digits"); if (v) digits = atoi(v); }
    if (digits < 0)  digits = 0;
    if (digits > 6)  digits = 6;        /* au-delà, l'axe entier déborderait */
    double facteur = 1.0;
    for (int i = 0; i < digits; ++i) facteur *= 10.0;

    QSlider *sl = new QSlider(Qt::Horizontal);
    sl->setMinimum((int)(vmin * facteur));
    sl->setMaximum((int)(vmax * facteur));
    sl->setSingleStep((int)((vstep > 0 ? vstep : 1) * facteur) > 0
                      ? (int)((vstep > 0 ? vstep : 1) * facteur) : 1);
        /* nearbyint, pas « +0.5 » : printf arrondit au PAIR le plus proche, donc
     * « %.0f » de 2.5 rend 2 — c'est ce que fait l'étalon. Ajouter 0.5 rendait
     * 3, et l'écart se voyait au banc. */
    sl->setValue((int) nearbyint(vval * facteur));
    sl->setProperty("sermoDigits", digits);

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  sl->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) sl->setMinimumHeight(atoi(v));
    }

    return (GtkWidget *)sl;
}

gchar *widget_hscale_envvar_construct(GtkWidget *widget)
{
    QSlider *sl = static_cast<QSlider *>(widget);
    if (!sl) return g_strdup("0");
    /* Règle de l'étalon : « %.Nf » avec N = digits. QString::number ne suit
     * pas la locale — le séparateur reste le point, partout. */
    int digits = sl->property("sermoDigits").toInt();
    double facteur = 1.0;
    for (int i = 0; i < digits; ++i) facteur *= 10.0;
    QByteArray t = QString::number(sl->value() / facteur, 'f', digits).toUtf8();
    return g_strdup(t.constData());
}

gchar *widget_hscale_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_hscale_envvar_construct(var->Widget);
}

void widget_hscale_clear(variable *var)
{
    if (!var || !var->Widget) return;
    QSlider *sl = static_cast<QSlider *>(var->Widget);
    sl->setValue(sl->minimum());
}

void widget_hscale_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_hscale_fileselect(variable *var, const char *n, const char *v) {}
void widget_hscale_removeselected(variable *var) {}
void widget_hscale_save(variable *var) {}
