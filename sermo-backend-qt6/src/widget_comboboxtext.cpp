/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_comboboxtext.cpp — liste déroulante Qt6
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <comboboxtext> → QComboBox ; sert aussi <comboboxentry>, seule à avoir un
 * champ de saisie (setEditable), comme chez l'étalon gtk3sermo.
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
#include "jouer_actions.h"

#include <QtWidgets/QComboBox>
#include <QtCore/QSignalBlocker>

/* Règle de l'étalon gtk3sermo : les <action> de la liste jouent (signal
 * « changed ») quand son texte n'est plus le même après qu'avant. Les signaux
 * de Qt sont tus pendant clear:, refresh: et removeselected: ; la comparaison
 * décide seule. Jusqu'à la 3.3.0 ce port ne jouait jamais ces actions. */
static void jouer_si_change(variable *var, QComboBox *cb, const QString &avant)
{
    if (avant != cb->currentText())
        jouer_actions((GtkWidget *)cb, var->Attributes, "changed", NULL);
}

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_comboboxtext_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QComboBox *cb = new QComboBox();
    /* Champ de saisie pour <comboboxentry> seulement : une <comboboxtext> de
     * l'étalon n'en a pas. Jusqu'à la 3.3.0 les deux étaient éditables ici, et
     * une frappe y rendait un texte que gtk3sermo ne peut pas rendre. */
    cb->setEditable(Type == WIDGET_COMBOBOXENTRY);

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
    QComboBox *cb = static_cast<QComboBox *>(var->Widget);
    QString avant = cb->currentText();
    {
        QSignalBlocker muet(cb);
        cb->clear();
        cb->clearEditText();
    }
    jouer_si_change(var, cb, avant);
}

void widget_comboboxtext_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    QComboBox *cb = static_cast<QComboBox *>(var->Widget);

    /* Le cœur neutre ne retient pas « _initialised » (g_object_set_data y est
     * sans effet) : le premier passage est noté sur le widget lui-même. */
    bool initialise = cb->property("sermoInitialise").toBool();
    QString avant = cb->currentText();
    QSignalBlocker muet(cb);

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
        /* Les actions ne sont branchées qu'ici, après le premier chargement,
         * comme sur l'étalon : rien ne joue au démarrage, même si <default>
         * choisit un élément. Ensuite : un élément choisi, ou une frappe dans
         * le champ d'une <comboboxentry>. */
        AttributeSet *Attr = var->Attributes;
        QObject::connect(cb, &QComboBox::currentTextChanged, cb,
            [cb, Attr](const QString &) {
                jouer_actions((GtkWidget *)cb, Attr, "changed", NULL); });
        return;
    }
    muet.unblock();
    jouer_si_change(var, cb, avant);
}

void widget_comboboxtext_fileselect(variable *var, const char *n, const char *v) {}
void widget_comboboxtext_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    QComboBox *cb = static_cast<QComboBox *>(var->Widget);
    QString avant = cb->currentText();
    {
        /* Comme l'étalon : l'élément choisi est retiré, puis le précédent est
         * choisi (le premier s'il n'y en a pas). Sans effet jusqu'à la 3.3.0. */
        QSignalBlocker muet(cb);
        int idx = cb->currentIndex();
        if (idx >= 0) cb->removeItem(idx);
        idx = idx > 0 ? idx - 1 : 0;
        cb->setCurrentIndex(idx < cb->count() ? idx : -1);
        if (cb->currentIndex() < 0) cb->clearEditText();
    }
    jouer_si_change(var, cb, avant);
}
void widget_comboboxtext_save(variable *var) {}
