/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_expander.cpp — Zone dépliable Qt6
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <expander> → QGroupBox avec setCheckable(true) qui agit comme expander.
 * Qt6 n'a pas de QExpander natif ; QGroupBox checkable est le plus proche.
 * Export : "true" si déplié (checked), "false" sinon.
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
#include "widget_expander.h"

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>

#include <string.h>
#include <strings.h>
#include <stdlib.h>


/* Même règle que l'étalon (gtk3 widget_expander.c) : l'état initial vient de
 * l'ATTRIBUT DE BALISE expanded= — « true », « yes » ou 1 — et un expander sans
 * cet attribut est REPLIÉ. Ce port lisait <default> (que l'étalon ignore) et
 * partait ouvert : le cas 40 du banc mesure les deux écarts. */
static bool expander_ouvert_au_depart(tag_attr *attr)
{
    const char *v = attr ? get_tag_attribute(attr, "expanded") : NULL;
    if (!v) return false;
    return (strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 || atoi(v) == 1);
}

GtkWidget *widget_expander_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *label = "Expander";
    if (Attr) {
        GList *element = NULL;
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) label = lbl;
    }

    QGroupBox   *gb     = new QGroupBox(QString::fromUtf8(label));
    QVBoxLayout *layout = new QVBoxLayout(gb);
    layout->setContentsMargins(4, 4, 4, 4);
    gb->setCheckable(true);
    gb->setChecked(expander_ouvert_au_depart(attr));

    return (GtkWidget *)gb;
}

gchar *widget_expander_envvar_construct(GtkWidget *widget)
{
    QGroupBox *gb = static_cast<QGroupBox *>(widget);
    if (!gb) return g_strdup("false");
    return g_strdup(gb->isChecked() ? "true" : "false");
}

gchar *widget_expander_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_expander_envvar_construct(var->Widget);
}

void widget_expander_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QGroupBox *>(var->Widget)->setChecked(false);
}

void widget_expander_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_expander_fileselect(variable *var, const char *n, const char *v) {}
void widget_expander_removeselected(variable *var) {}
void widget_expander_save(variable *var) {}
