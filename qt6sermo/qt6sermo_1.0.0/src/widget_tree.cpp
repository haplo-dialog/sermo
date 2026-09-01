/*
 * widget_tree.cpp — Arbre Qt6 (QTreeWidget)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <tree> → QTreeWidget avec une colonne
 * Alimentation via <input>cmd</input> : format "parent|enfant" ou juste "item"
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
#include "widget_tree.h"
#include "safe_exec.h"

#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void populate_from_command(QTreeWidget *tw, const char *cmd)
{
    FILE *fp = widget_opencommand(cmd);
    if (!fp) return;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (!len) continue;

        /* Format "parent|enfant" */
        char *sep = strchr(line, '|');
        if (sep) {
            *sep = '\0';
            const char *parent_text = line;
            const char *child_text  = sep + 1;
            /* Chercher ou créer le parent */
            QList<QTreeWidgetItem *> found = tw->findItems(
                QString::fromUtf8(parent_text), Qt::MatchExactly | Qt::MatchRecursive);
            QTreeWidgetItem *parent_item;
            if (!found.isEmpty()) {
                parent_item = found.first();
            } else {
                parent_item = new QTreeWidgetItem(tw, QStringList(QString::fromUtf8(parent_text)));
            }
            new QTreeWidgetItem(parent_item, QStringList(QString::fromUtf8(child_text)));
        } else {
            new QTreeWidgetItem(tw, QStringList(QString::fromUtf8(line)));
        }
    }
    fclose(fp);  /* safe_popen() uses fdopen() — pclose() is UB */
}

GtkWidget *widget_tree_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QTreeWidget *tw = new QTreeWidget();
    tw->setColumnCount(1);
    tw->setHeaderHidden(true);

    if (Attr) {
        GList *element = NULL;
        gchar *cmd = attributeset_get_first(&element, Attr, ATTR_INPUT);
        if (cmd && *cmd) populate_from_command(tw, cmd);

        element = NULL;
        gchar *item = attributeset_get_first(&element, Attr, ATTR_ITEM);
        while (item) {
            if (*item) new QTreeWidgetItem(tw, QStringList(QString::fromUtf8(item)));
            item = attributeset_get_next(&element, Attr, ATTR_ITEM);
        }
    }

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
    if (var->Attributes) {
        GList *element = NULL;
        gchar *cmd = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
        if (cmd && *cmd) {
            tw->clear();
            populate_from_command(tw, cmd);
        }
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
