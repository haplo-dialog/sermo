/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_comboboxtext.cpp — ComboBox éditable Qt6
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <comboboxtext> → QComboBox avec setEditable(true) ; sert aussi <comboboxentry>.
 * Alimentation : <input> (commande ou fichier, un élément par ligne — une
 * ligne vide fait un élément vide, comme chez l'étalon)
 * puis <item>, chargés par widget_comboboxtext_refresh().
 * Export : texte courant (saisi ou sélectionné)
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
#include "widget_comboboxtext.h"
#include "safe_exec.h"

#include <QtWidgets/QComboBox>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_comboboxtext_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QComboBox *cb = new QComboBox();
    cb->setEditable(true);

    /* Éléments, sélection et <default> : posés par widget_comboboxtext_refresh(),
     * que le cœur appelle juste après la création. Lire <input> ici aussi
     * exécuterait la commande deux fois, et un refresh ultérieur doit de toute
     * façon savoir tout recharger. */

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  cb->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) cb->setMinimumHeight(atoi(v));
    }

    return (GtkWidget *)cb;
}

gchar *widget_comboboxtext_envvar_construct(GtkWidget *widget)
{
    QComboBox *cb = static_cast<QComboBox *>(widget);
    if (!cb) return g_strdup("");
    return g_strdup(cb->currentText().toUtf8().constData());
}

gchar *widget_comboboxtext_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_comboboxtext_envvar_construct(var->Widget);
}

void widget_comboboxtext_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QComboBox *>(var->Widget)->clear();
}

void widget_comboboxtext_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    QComboBox *cb = static_cast<QComboBox *>(var->Widget);

    /* Le cœur neutre ne retient pas « _initialised » (g_object_set_data y est
     * sans effet) : le premier passage est noté sur le widget lui-même. */
    bool initialise = cb->property("sermoInitialise").toBool();

    /* Même déroulé que l'étalon gtk3sermo. Un refresh ultérieur recharge tout :
     * on vide d'abord, zone de saisie comprise. */
    if (initialise) {
        cb->clear();
        cb->clearEditText();
    }

    if (var->Attributes) {
        /* <input> d'abord : un élément par ligne, lignes vides comprises —
         * l'étalon gtk3sermo les garde (mesuré : « \nun\n » sélectionne
         * l'élément vide). */
        gchar **lignes = widget_input_lines(var->Attributes);
        if (lignes) {
            for (gchar **ligne = lignes; *ligne; ligne++)
                cb->addItem(QString::fromUtf8(*ligne));
            g_strfreev(lignes);
        }
        /* <item> ensuite. */
        GList *element = NULL;
        gchar *item = attributeset_get_first(&element, var->Attributes, ATTR_ITEM);
        while (item) {
            if (*item) cb->addItem(QString::fromUtf8(item));
            item = attributeset_get_next(&element, var->Attributes, ATTR_ITEM);
        }
    }

    if (var->Type == WIDGET_COMBOBOXENTRY) {
        /* Parite gtk3 : <comboboxentry> ne selectionne rien (rend ""), alors que
         * QComboBox editable prend l'item 0 d'office. <comboboxtext>, lui,
         * garde le premier (distinction gtkdialog). */
        cb->setCurrentIndex(-1);
        cb->clearEditText();
    } else {
        cb->setCurrentIndex(0);   /* sans effet si la liste est vide */
    }

    if (!initialise) {
        /* <default>, au premier passage seulement : l'élément de même texte.
         * Absent de la liste, il ne s'écrit que dans <comboboxentry> — une
         * <comboboxtext> de l'étalon n'a pas de zone de saisie. */
        if (var->Attributes) {
            GList *element = NULL;
            gchar *def = attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT);
            if (def) {
                int idx = cb->findText(QString::fromUtf8(def));
                if (idx >= 0)
                    cb->setCurrentIndex(idx);
                else if (var->Type == WIDGET_COMBOBOXENTRY)
                    cb->setEditText(QString::fromUtf8(def));
            }
        }
        cb->setProperty("sermoInitialise", true);
    }
}

void widget_comboboxtext_fileselect(variable *var, const char *n, const char *v) {}
void widget_comboboxtext_removeselected(variable *var) {}
void widget_comboboxtext_save(variable *var) {}
