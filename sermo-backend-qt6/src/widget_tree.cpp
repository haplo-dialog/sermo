/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_tree.cpp — Arbre Qt6 (QTreeWidget)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <tree> → QTreeWidget, une colonne (plus si les lignes de <input> en apportent)
 * Alimentation : <item> à la création ; <input> (commande ou fichier) lu par
 * widget_tree_refresh() — une rangée par ligne, colonnes séparées par « | »
 * Export : texte de l'item sélectionné (1re colonne)
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
#include "widget_tree.h"
#include "safe_exec.h"

#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Une rangée par ligne de <input>, colonnes séparées par « | » — le format de
 * l'étalon gtk3sermo et du manuel. (qt6 lisait « parent|enfant » comme une
 * hiérarchie, format que les autres ports ne connaissent pas.) */
static void add_rows_from_input(QTreeWidget *tw, AttributeSet *Attr)
{
    gchar **lignes = widget_input_lines(Attr);
    if (!lignes) return;
    for (gchar **ligne = lignes; *ligne; ligne++) {
        QStringList cols = QString::fromUtf8(*ligne).split('|');
        if (tw->columnCount() < cols.size()) tw->setColumnCount(cols.size());
        new QTreeWidgetItem(tw, cols);
    }
    g_strfreev(lignes);
}

static void add_rows_from_items(QTreeWidget *tw, AttributeSet *Attr)
{
    GList *element = NULL;
    gchar *item = attributeset_get_first(&element, Attr, ATTR_ITEM);
    while (item) {
        if (*item) new QTreeWidgetItem(tw, QStringList(QString::fromUtf8(item)));
        item = attributeset_get_next(&element, Attr, ATTR_ITEM);
    }
}

GtkWidget *widget_tree_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QTreeWidget *tw = new QTreeWidget();
    tw->setColumnCount(1);
    tw->setHeaderHidden(true);

    /* <input> : lu par widget_tree_refresh(), que le cœur appelle juste après
     * la création — le lire aussi ici exécuterait la commande deux fois. */
    if (Attr) add_rows_from_items(tw, Attr);

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  tw->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) tw->setMinimumHeight(atoi(v));
    }

    /* Parite gtk3 : selectionner le premier item si rien ne l'est. */
    if (!tw->currentItem() && tw->topLevelItemCount() > 0)
        tw->setCurrentItem(tw->topLevelItem(0));
    return (GtkWidget *)tw;
}

gchar *widget_tree_envvar_construct(GtkWidget *widget)
{
    QTreeWidget *tw = static_cast<QTreeWidget *>(widget);
    if (!tw) return g_strdup("");
    QTreeWidgetItem *cur = tw->currentItem();
    if (!cur) return g_strdup("");
    return g_strdup(cur->text(0).toUtf8().constData());
}

gchar *widget_tree_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_tree_envvar_construct(var->Widget);
}

void widget_tree_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QTreeWidget *>(var->Widget)->clear();
}

void widget_tree_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    QTreeWidget *tw = static_cast<QTreeWidget *>(var->Widget);

    /* Avec <input>, le refresh de l'étalon gtk3sermo VIDE l'arbre puis le
     * recharge : <input> d'abord, <item> ensuite. Sans <input>, rien à
     * recharger : les <item> posés à la création restent tels quels. */
    if (var->Attributes && attributeset_is_avail(var->Attributes, ATTR_INPUT)) {
        tw->clear();
        add_rows_from_input(tw, var->Attributes);
        add_rows_from_items(tw, var->Attributes);
    }

    /* Au premier passage, même règle de sélection qu'à la création. Le cœur
     * neutre ne retient pas « _initialised » : le passage est noté sur le
     * widget. */
    if (!tw->property("sermoInitialise").toBool()) {
        if (!tw->currentItem() && tw->topLevelItemCount() > 0)
            tw->setCurrentItem(tw->topLevelItem(0));
        tw->setProperty("sermoInitialise", true);
    }
    tw->update();
}

void widget_tree_fileselect(variable *var, const char *n, const char *v) {}

void widget_tree_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    QTreeWidget     *tw  = static_cast<QTreeWidget *>(var->Widget);
    QTreeWidgetItem *cur = tw->currentItem();
    if (cur) delete cur;
}

void widget_tree_save(variable *var) {}
