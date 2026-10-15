/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_statusbar.cpp — Barre de statut Qt6 (QStatusBar)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <statusbar> → QStatusBar
 * Le texte vit dans un QLabel permanent (propriété labelWidget).
 * Export : dernier texte affiché.
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
#include "widget_statusbar.h"

#include <QtWidgets/QStatusBar>
#include <QtWidgets/QLabel>

#include <string.h>
#include <stdlib.h>

/* QStatusBar ne mémorise pas le texte courant facilement ; on utilise
 * un QLabel intégré comme widget permanent pour le lire. */

GtkWidget *widget_statusbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QStatusBar *sb    = new QStatusBar();
    QLabel     *label = new QLabel();
    sb->addWidget(label, 1);

    if (Attr) {
        GList *element = NULL;
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        /* gtk3sermo pousse <label> puis <default> : le texte montré (et la variable)
         * est <default> s'il est là. Ignoré jusqu'à la 2.8.0 : barre vide. */
        GList *el_def = NULL;
        gchar *def = attributeset_get_first(&el_def, Attr, ATTR_DEFAULT);
        if (def && *def) lbl = def;
        if (lbl && *lbl) {
            /* Le libellé seul : un showMessage() du même texte s'écrivait
             * par-dessus (« Prêt » doublé, capture du site 2.8.0). */
            label->setText(QString::fromUtf8(lbl));
        }
    }

    /* Stocker le label comme userData via setProperty */
    sb->setProperty("labelWidget", QVariant::fromValue((QObject*)label));

    return (GtkWidget *)sb;
}

gchar *widget_statusbar_envvar_construct(GtkWidget *widget)
{
    QStatusBar *sb = static_cast<QStatusBar *>(widget);
    if (!sb) return g_strdup("");
    QObject *obj = sb->property("labelWidget").value<QObject *>();
    QLabel  *lbl = qobject_cast<QLabel *>(obj);
    if (!lbl) return g_strdup("");
    return g_strdup(lbl->text().toUtf8().constData());
}

gchar *widget_statusbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_statusbar_envvar_construct(var->Widget);
}

void widget_statusbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    QStatusBar *sb = static_cast<QStatusBar *>(var->Widget);
    sb->clearMessage();
    QObject *obj = sb->property("labelWidget").value<QObject *>();
    QLabel  *lbl = qobject_cast<QLabel *>(obj);
    if (lbl) lbl->clear();
}

/* <input> à chaque « refresh », comme l'étalon : la première ligne est le
 * message. Au premier passage, un <label> ou un <default> l'emporte (l'étalon
 * les pousse après) ; la commande tourne quand même. Aucune <action> n'est
 * branchée sur ce widget dans ce port. Jusqu'au 2026-10-13, l'<input> n'était
 * jamais lu (ST restait vide). */
void widget_statusbar_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    QStatusBar *sb = static_cast<QStatusBar *>(var->Widget);
    bool premier = !sb->property("sermoLu").toBool();
    sb->setProperty("sermoLu", true);
    gchar *ligne = widget_input_line(var->Attributes);
    if (!ligne) return;
    GList *el = NULL, *el_def = NULL;
    QLabel *lbl = qobject_cast<QLabel *>(sb->property("labelWidget").value<QObject *>());
    if (lbl && !(premier && (attributeset_get_first(&el, var->Attributes, ATTR_LABEL) ||
                             attributeset_get_first(&el_def, var->Attributes, ATTR_DEFAULT))))
        lbl->setText(QString::fromUtf8(ligne));
    g_free(ligne);
}

void widget_statusbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_statusbar_removeselected(variable *var) {}
void widget_statusbar_save(variable *var) {}
