/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_vscale.cpp — Glissière verticale Qt6 (QSlider)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_vscale.h"
#include <cmath>
#include <QtCore/QString>
#include <QtCore/QByteArray>

#include <QtWidgets/QSlider>
#include <QtCore/Qt>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_vscale_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    double vmin = 0.0, vmax = 100.0, vstep = 1.0, vval = 0.0;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "range-min")) || (v = get_tag_attribute(attr, "min")))   vmin  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-max")) || (v = get_tag_attribute(attr, "max")))   vmax  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-step")) || (v = get_tag_attribute(attr, "step"))) vstep = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "value"))) vval  = g_ascii_strtod(v, NULL);
    }
    if (Attr) {
        GList *element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) vval = g_ascii_strtod(def, NULL);
    }
    vval = widget_command_value(Attr, vval);   /* <input>echo N</input> prioritaire */

    /* Voir widget_hscale.cpp : QSlider est entier, on met l'axe à l'échelle
     * de 10^digits pour honorer l'attribut digits=. */
    int digits = 0;
    if (attr) { const char *v = get_tag_attribute(attr, "digits"); if (v) digits = atoi(v); }
    if (digits < 0)  digits = 0;
    if (digits > 6)  digits = 6;
    double facteur = 1.0;
    for (int i = 0; i < digits; ++i) facteur *= 10.0;

    QSlider *sl = new QSlider(Qt::Vertical);
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

gchar *widget_vscale_envvar_construct(GtkWidget *widget)
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

gchar *widget_vscale_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_vscale_envvar_construct(var->Widget);
}

void widget_vscale_clear(variable *var)
{
    if (!var || !var->Widget) return;
    QSlider *sl = static_cast<QSlider *>(var->Widget);
    sl->setValue(sl->minimum());
}

void widget_vscale_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_vscale_fileselect(variable *var, const char *n, const char *v) {}
void widget_vscale_removeselected(variable *var) {}
void widget_vscale_save(variable *var) {}
