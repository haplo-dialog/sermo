/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_table.cpp — Tableau Qt6 (QTreeWidget multi-colonnes)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <table> → QTreeWidget (liste plate multi-colonnes)
 * En-têtes : <label>col1|col2</label>
 * Rangées : chaque <item> à la création, puis chaque ligne de <input> (commande
 * ou fichier) lue par widget_table_refresh() — colonnes séparées par « | »
 * Export : colonne « exported-column » (défaut 0) de la ligne sélectionnée
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
#include "widget_table.h"
#include "safe_exec.h"

#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtCore/QStringList>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Une rangée, colonnes séparées par « | » : même règle pour un <item> et pour
 * une ligne de <input>, comme chez l'étalon gtk3sermo. (qt6 lisait <input> en
 * TSV et prenait sa 1re ligne pour les en-têtes.) */
static void add_row(QTreeWidget *tw, const char *row)
{
    QStringList c = QString::fromUtf8(row).split('|');
    if (tw->columnCount() < c.size()) tw->setColumnCount(c.size());
    new QTreeWidgetItem(tw, c);
}

GtkWidget *widget_table_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QTreeWidget *tw = new QTreeWidget();
    tw->setRootIsDecorated(false);
    tw->setAlternatingRowColors(true);

    if (Attr) {
        /* En-tetes : <label>col1|col2</label> (parite gtk3, PAS le 1er <item>). */
        GList *element = NULL;
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) {
            QStringList cols = QString::fromUtf8(lbl).split('|');
            tw->setColumnCount(cols.size());
            tw->setHeaderLabels(cols);
        }
        /* Lignes : CHAQUE <item> (colonnes separees par |). */
        element = NULL;
        gchar *row = attributeset_get_first(&element, Attr, ATTR_ITEM);
        while (row) {
            if (*row) add_row(tw, row);
            row = attributeset_get_next(&element, Attr, ATTR_ITEM);
        }
        /* <input> : lu par widget_table_refresh(), que le cœur appelle juste
         * après la création — le lire aussi ici exécuterait la commande deux
         * fois. */
    }

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  tw->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) tw->setMinimumHeight(atoi(v));
        if ((v = get_tag_attribute(attr, "exported-column")))
            tw->setProperty("exported_column", atoi(v));
    }

    /* Parite gtk3 : selectionner la premiere ligne si rien ne l'est. */
    if (!tw->currentItem() && tw->topLevelItemCount() > 0)
        tw->setCurrentItem(tw->topLevelItem(0));
    return (GtkWidget *)tw;
}

gchar *widget_table_envvar_construct(GtkWidget *widget)
{
    QTreeWidget *tw = static_cast<QTreeWidget *>(widget);
    if (!tw) return g_strdup("");
    QTreeWidgetItem *cur = tw->currentItem();
    if (!cur) return g_strdup("");
    /* Parite gtk3 : rend la colonne « exported-column » (defaut 0), pas la
     * ligne entiere jointe. */
    int col = tw->property("exported_column").toInt();
    if (col < 0 || col >= cur->columnCount()) col = 0;
    return g_strdup(cur->text(col).toUtf8().constData());
}

gchar *widget_table_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_table_envvar_construct(var->Widget);
}

void widget_table_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QTreeWidget *>(var->Widget)->clear();
}

void widget_table_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    QTreeWidget *tw = static_cast<QTreeWidget *>(var->Widget);

    /* <input> (commande ou fichier) : une rangée par ligne, AJOUTÉE aux
     * rangées présentes — le refresh de l'étalon gtk3sermo ne vide pas le
     * tableau, c'est le rôle de l'action clear. */
    gchar **lignes = widget_input_lines(var->Attributes);
    if (lignes) {
        for (gchar **ligne = lignes; *ligne; ligne++)
            add_row(tw, *ligne);
        g_strfreev(lignes);
    }

    /* Au premier passage, les rangées venues de <input> suivent la règle de
     * sélection que la création applique aux <item>. Le cœur neutre ne retient
     * pas « _initialised » : le passage est noté sur le widget. */
    if (!tw->property("sermoInitialise").toBool()) {
        if (!tw->currentItem() && tw->topLevelItemCount() > 0)
            tw->setCurrentItem(tw->topLevelItem(0));
        tw->setProperty("sermoInitialise", true);
    }
    tw->update();
}

void widget_table_fileselect(variable *var, const char *n, const char *v) {}

void widget_table_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    QTreeWidget     *tw  = static_cast<QTreeWidget *>(var->Widget);
    QTreeWidgetItem *cur = tw->currentItem();
    if (cur) delete cur;
}

void widget_table_save(variable *var) {}
