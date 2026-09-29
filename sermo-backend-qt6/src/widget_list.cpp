/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_list.cpp — Liste de sélection Qt6 (QListWidget)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <list> → QListWidget
 * Alimentation : <item> à la création ; <input> (commande ou fichier, une
 * rangée par ligne) lu par widget_list_refresh()
 * Export : item(s) sélectionné(s) séparés par "|"
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
#include "widget_list.h"
#include "safe_exec.h"

#include <QtWidgets/QListWidget>
#include <QtCore/Qt>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_list_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QListWidget *lw = new QListWidget();

    /* Sélection multiple si demandée */
    if (attr) {
        const char *v = get_tag_attribute(attr, "multiple-selection");
        if (v && (strcmp(v,"true")==0 || strcmp(v,"1")==0))
            lw->setSelectionMode(QAbstractItemView::MultiSelection);
        if ((v = get_tag_attribute(attr, "width-request")))  lw->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) lw->setMinimumHeight(atoi(v));
    }

    if (Attr) {
        GList *element = NULL;
        gchar *item = attributeset_get_first(&element, Attr, ATTR_ITEM);
        while (item) {
            if (*item) lw->addItem(QString::fromUtf8(item));
            item = attributeset_get_next(&element, Attr, ATTR_ITEM);
        }
        /* <input> : lu par widget_list_refresh(), que le cœur appelle juste
         * après la création — le lire aussi ici exécuterait la commande deux
         * fois. */

        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) {
            QList<QListWidgetItem *> found = lw->findItems(QString::fromUtf8(def), Qt::MatchExactly);
            if (!found.isEmpty()) lw->setCurrentItem(found.first());
        }
    }

    /* Parite gtk3 (GtkListBox auto-selectionne la 1re ligne) : si rien n'est
     * selectionne, selectionner le premier item. */
    if (lw->selectedItems().isEmpty() && lw->count() > 0)
        lw->setCurrentRow(0);
    return (GtkWidget *)lw;
}

gchar *widget_list_envvar_construct(GtkWidget *widget)
{
    QListWidget *lw = static_cast<QListWidget *>(widget);
    if (!lw) return g_strdup("");

    QList<QListWidgetItem *> sel = lw->selectedItems();
    if (sel.isEmpty()) return g_strdup("");

    QString result;
    for (int i = 0; i < sel.size(); ++i) {
        if (i > 0) result += "|";
        result += sel.at(i)->text();
    }
    return g_strdup(result.toUtf8().constData());
}

gchar *widget_list_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_list_envvar_construct(var->Widget);
}

void widget_list_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QListWidget *>(var->Widget)->clear();
}

void widget_list_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    QListWidget *lw = static_cast<QListWidget *>(var->Widget);

    /* <input> (commande ou fichier) : une rangée par ligne, AJOUTÉE aux
     * rangées présentes — le refresh de l'étalon gtk3sermo ne vide pas la
     * liste, c'est le rôle de l'action clear. Jusqu'à la 2.7.0, la directive
     * brute (« Command:… », « file:… ») partait telle quelle vers le shell. */
    gchar **lignes = widget_input_lines(var->Attributes);
    if (lignes) {
        for (gchar **ligne = lignes; *ligne; ligne++)
            lw->addItem(QString::fromUtf8(*ligne));
        g_strfreev(lignes);
    }

    /* Au premier passage, les rangées venues de <input> suivent la règle de
     * sélection que la création applique aux <item>. Le cœur neutre ne retient
     * pas « _initialised » : le passage est noté sur le widget. */
    if (!lw->property("sermoInitialise").toBool()) {
        if (lw->selectedItems().isEmpty() && lw->count() > 0)
            lw->setCurrentRow(0);
        lw->setProperty("sermoInitialise", true);
    }
    lw->update();
}

void widget_list_fileselect(variable *var, const char *n, const char *v) {}

void widget_list_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    QListWidget *lw = static_cast<QListWidget *>(var->Widget);
    qDeleteAll(lw->selectedItems());
}

void widget_list_save(variable *var) {}
