/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_revealer.cpp — Un enfant qui se montre et se cache (Qt 6)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ Qt n'a pas de GtkRevealer : montrer/cacher est natif, la TRANSITION ne
 * l'est pas (elle demanderait une QPropertyAnimation sur maximumHeight). Le
 * port montre ou cache — l'état exporté, lui, est identique à l'étalon.
 * L'attribut transition= est donc accepté et ignoré ici : dire ce qu'on ne
 * fait pas vaut mieux que de le taire.
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_revealer.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QVariant>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

GtkWidget *widget_revealer_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;

    bool montre = false;
    if (attr) {
        const char *v = get_tag_attribute(attr, "reveal");
        if (v && (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || atoi(v) == 1))
            montre = true;
    }
    if (Attr && attributeset_is_avail(Attr, ATTR_DEFAULT)) {
        GList *el = nullptr;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (!strcasecmp(def, "true") || !strcasecmp(def, "yes") || atoi(def) == 1))
            montre = true;
    }

    QWidget     *hote   = new QWidget();
    QVBoxLayout *vertic = new QVBoxLayout(hote);
    vertic->setContentsMargins(0, 0, 0, 0);

    stackelement s = pop();
    int poses = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        if (poses == 0) vertic->addWidget(static_cast<QWidget *>(s.widgets[n]));
        else fprintf(stderr, "qt6sermo: <revealer> ne prend QU'UN enfant : le %de "
                             "est ignoré. Emballer le surplus dans une <vbox>.\n",
                     poses + 1);
        poses++;
    }
    hote->setVisible(montre);
    hote->setProperty("sermoMontre", montre);
    return (GtkWidget *) hote;
}

/* Export : l'état courant, « true » ou « false » (étalon gtk3). */
gchar *widget_revealer_envvar_construct(GtkWidget *widget)
{
    QWidget *w = static_cast<QWidget *>(widget);
    if (!w) return g_strdup("false");
    return g_strdup(w->property("sermoMontre").toBool() ? "true" : "false");
}
gchar *widget_revealer_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return nullptr;
    return widget_revealer_envvar_construct(var->Widget);
}
void widget_revealer_clear(variable *var)
{
    if (!var || !var->Widget) return;
    QWidget *w = static_cast<QWidget *>(var->Widget);
    w->setVisible(false);
    w->setProperty("sermoMontre", false);
}
void widget_revealer_refresh(variable *var)        { (void) var; }
void widget_revealer_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_revealer_removeselected(variable *var) { (void) var; }
void widget_revealer_save(variable *var)           { (void) var; }
