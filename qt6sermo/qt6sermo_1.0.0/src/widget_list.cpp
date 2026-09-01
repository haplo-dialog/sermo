/*
 * widget_list.cpp — Liste de sélection Qt6 (QListWidget)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <list> → QListWidget
 * Alimentation : <item> + <input>cmd</input>
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

static void add_items_from_command(QListWidget *lw, const char *cmd)
{
    FILE *fp = widget_opencommand(cmd);
    if (!fp) return;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (len > 0)
            lw->addItem(QString::fromUtf8(line));
    }
    fclose(fp);  /* safe_popen() uses fdopen() — pclose() is UB */
}

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

        element = NULL;
        gchar *cmd = attributeset_get_first(&element, Attr, ATTR_INPUT);
        if (cmd && *cmd) add_items_from_command(lw, cmd);

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
    if (var->Attributes) {
        GList *element = NULL;
        gchar *cmd = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
        if (cmd && *cmd) {
            lw->clear();
            add_items_from_command(lw, cmd);
        }
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
