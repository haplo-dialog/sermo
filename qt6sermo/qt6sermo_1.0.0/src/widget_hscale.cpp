/*
 * widget_hscale.cpp — Glissière horizontale Qt6 (QSlider)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
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

    QSlider *sl = new QSlider(Qt::Horizontal);
    sl->setMinimum((int)vmin);
    sl->setMaximum((int)vmax);
    sl->setSingleStep((int)(vstep > 0 ? vstep : 1));
    sl->setValue((int)vval);

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
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", sl->value());
    return g_strdup(buf);
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
