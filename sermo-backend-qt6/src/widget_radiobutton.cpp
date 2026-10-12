/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_radiobutton.cpp — Bouton radio Qt6 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_radiobutton.h"
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QButtonGroup>
#include <string.h>
#include <stdlib.h>


GtkWidget *widget_radiobutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = nullptr;
    int w = 120, h = 28;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }
    gchar *label = nullptr;
    if (Attr) label = attributeset_get_first(&element, Attr, ATTR_LABEL);
    QRadioButton *rb = new QRadioButton(label ? label : "");
    rb->resize(w, h);
    /* Un groupe par suite de boutons radio, comme gtk3sermo : le cœur ouvre le
     * groupe (lastradiowidget) et le ferme dès qu'un autre widget est créé. Qt
     * regroupait par conteneur parent (deux questions dans une même boîte ne
     * faisaient qu'un groupe), et ne cochait aucun bouton d'office. */
    if (lastradiowidget == nullptr) {
        QButtonGroup *g = new QButtonGroup(rb);
        g->setExclusive(true);
        rb->setProperty("sermoGroupe", QVariant::fromValue((QObject *)g));
        lastradiowidget = (GtkWidget *)rb;
        rb->setChecked(true);               /* le 1er coché, comme GtkRadioButton */
    } else {
        QObject *g = static_cast<QRadioButton *>((void *)lastradiowidget)->property("sermoGroupe").value<QObject *>();
        rb->setProperty("sermoGroupe", QVariant::fromValue(g));
    }
    rb->setAutoExclusive(false);
    if (QButtonGroup *g = qobject_cast<QButtonGroup *>(rb->property("sermoGroupe").value<QObject *>()))
        g->addButton(rb);
    if (Attr) {
        element = nullptr;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && (g_ascii_strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
            rb->setChecked(true);
    }
    return (GtkWidget *)rb;
}

gchar *widget_radiobutton_envvar_construct(GtkWidget *widget)
{
    QRadioButton *rb = static_cast<QRadioButton*>(widget);
    if (!rb) return g_strdup("false");
    return g_strdup(rb->isChecked() ? "true" : "false");
}
gchar *widget_radiobutton_envvar_all_construct(variable *var) { if (!var || !var->Widget) return nullptr; return widget_radiobutton_envvar_construct(var->Widget); }
void   widget_radiobutton_clear(variable *var) { if (var && var->Widget) static_cast<QRadioButton*>(var->Widget)->setChecked(false); }
void   widget_radiobutton_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget*>(var->Widget)->update(); }
void   widget_radiobutton_fileselect(variable *var, const char *n, const char *v) {}
void   widget_radiobutton_removeselected(variable *var) {}
void   widget_radiobutton_save(variable *var) {}
