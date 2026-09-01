/*
 * widget_comboboxtext.cpp — ComboBox éditable Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <comboboxtext> → QComboBox avec setEditable(true)
 * Même alimentation que widget_combobox mais l'utilisateur peut saisir du texte.
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

static void add_items_from_command(QComboBox *cb, const char *cmd)
{
    FILE *fp = widget_opencommand(cmd);
    if (!fp) return;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (len > 0)
            cb->addItem(QString::fromUtf8(line));
    }
    fclose(fp);  /* safe_popen() uses fdopen() — pclose() is UB */
}

GtkWidget *widget_comboboxtext_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QComboBox *cb = new QComboBox();
    cb->setEditable(true);

    if (Attr) {
        GList *element = NULL;
        gchar *item = attributeset_get_first(&element, Attr, ATTR_ITEM);
        while (item) {
            if (*item) cb->addItem(QString::fromUtf8(item));
            item = attributeset_get_next(&element, Attr, ATTR_ITEM);
        }

        element = NULL;
        gchar *cmd = attributeset_get_first(&element, Attr, ATTR_INPUT);
        if (cmd && *cmd) add_items_from_command(cb, cmd);

        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) cb->setCurrentText(QString::fromUtf8(def));
        else if (Type == WIDGET_COMBOBOXENTRY) {
            /* Parite gtk3 : <comboboxentry> sans <default> ne selectionne rien
             * (rend ""), alors que QComboBox editable prend l'item 0 d'office.
             * <comboboxtext>, lui, garde le premier (distinction gtkdialog). */
            cb->setCurrentIndex(-1);
            cb->clearEditText();
        }
    }

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
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_comboboxtext_fileselect(variable *var, const char *n, const char *v) {}
void widget_comboboxtext_removeselected(variable *var) {}
void widget_comboboxtext_save(variable *var) {}
