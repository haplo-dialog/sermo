/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_combobox.cpp — Liste déroulante Qt6 (QComboBox)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <combobox> → QComboBox (non éditable)
 * Alimentation : <item>texte</item> ; <input> n'est pas implémenté (étalon)
 * Export : texte de l'item sélectionné
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
#include "widget_combobox.h"
#include "safe_exec.h"

#include <QtWidgets/QComboBox>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_combobox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QComboBox *cb = new QComboBox();
    cb->setEditable(false);

    if (Attr) {
        GList *element = NULL;
        gchar *item = attributeset_get_first(&element, Attr, ATTR_ITEM);
        while (item) {
            if (*item) cb->addItem(QString::fromUtf8(item));
            item = attributeset_get_next(&element, Attr, ATTR_ITEM);
        }

        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        int applied = -1;
        if (def && *def) {
            int idx = cb->findText(QString::fromUtf8(def));
            if (idx >= 0) applied = idx;
        }
        /* Parite gtk3 : sans <default>, AUCUNE selection (rend ""), au lieu du
         * premier item que QComboBox selectionne d'office. <comboboxtext>, lui,
         * selectionne le premier -- c'est une distinction reelle de gtkdialog. */
        cb->setCurrentIndex(applied);
    }

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  cb->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) cb->setMinimumHeight(atoi(v));
    }

    return (GtkWidget *)cb;
}

gchar *widget_combobox_envvar_construct(GtkWidget *widget)
{
    QComboBox *cb = static_cast<QComboBox *>(widget);
    if (!cb) return g_strdup("");
    return g_strdup(cb->currentText().toUtf8().constData());
}

gchar *widget_combobox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_combobox_envvar_construct(var->Widget);
}

void widget_combobox_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QComboBox *>(var->Widget)->clear();
}

void widget_combobox_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    QComboBox *cb = static_cast<QComboBox *>(var->Widget);
    /* <input> n'est PAS implémenté pour <combobox> chez l'étalon gtk3sermo : il
     * le signale et n'exécute rien. Jusqu'à la 2.7.0, qt6 lançait la directive
     * brute (« Command:… ») à la création puis ici, et le vidage/rechargement
     * sélectionnait d'office le premier élément. */
    if (var->Attributes) {
        GList *element = NULL;
        gchar *act = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
        while (act) {
            if (input_is_shell_command(act))
                g_warning("%s(): <input> not implemented for this widget.", __func__);
            else if (strncasecmp(act, "file:", 5) == 0 && act[5])
                g_warning("%s(): <input file> not implemented for this widget.", __func__);
            act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
        }
    }
    cb->update();
}

void widget_combobox_fileselect(variable *var, const char *n, const char *v) {}
void widget_combobox_removeselected(variable *var) {}
void widget_combobox_save(variable *var) {}
