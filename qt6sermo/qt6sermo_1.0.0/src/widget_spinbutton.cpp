/*
 * widget_spinbutton.cpp — Champ numérique Qt6 (QDoubleSpinBox)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <spinbutton> → QDoubleSpinBox
 * Attributs : value, min, max, step, digits (décimales)
 * Export : valeur courante en chaîne décimale
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
#include "widget_spinbutton.h"

#include <QtWidgets/QDoubleSpinBox>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_spinbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QDoubleSpinBox *sb = new QDoubleSpinBox();
    sb->setMinimum(0.0);
    sb->setMaximum(100.0);
    sb->setSingleStep(1.0);
    sb->setValue(0.0);
    sb->setDecimals(2);

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "range-min")) || (v = get_tag_attribute(attr, "min")))  sb->setMinimum(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "range-max")) || (v = get_tag_attribute(attr, "max")))  sb->setMaximum(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "range-step")) || (v = get_tag_attribute(attr, "step"))) sb->setSingleStep(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "value")))  sb->setValue(g_ascii_strtod(v, NULL));
        if ((v = get_tag_attribute(attr, "digits"))) sb->setDecimals(atoi(v));
        if ((v = get_tag_attribute(attr, "width-request")))  sb->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) sb->setMinimumHeight(atoi(v));
    }

    if (Attr) {
        GList *element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) sb->setValue(g_ascii_strtod(def, NULL));
        /* <input>echo N</input> prioritaire (NaN-safe : fallback = valeur actuelle). */
        sb->setValue(widget_command_value(Attr, sb->value()));
    }

    return (GtkWidget *)sb;
}

gchar *widget_spinbutton_envvar_construct(GtkWidget *widget)
{
    QDoubleSpinBox *sb = static_cast<QDoubleSpinBox *>(widget);
    if (!sb) return g_strdup("0");
    char buf[64];
    snprintf(buf, sizeof(buf), "%.6g", sb->value());
    return g_strdup(buf);
}

gchar *widget_spinbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_spinbutton_envvar_construct(var->Widget);
}

void widget_spinbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    QDoubleSpinBox *sb = static_cast<QDoubleSpinBox *>(var->Widget);
    sb->setValue(sb->minimum());
}

void widget_spinbutton_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_spinbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_spinbutton_removeselected(variable *var) {}
void widget_spinbutton_save(variable *var) {}
