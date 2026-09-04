/*
 * widget_table.cpp — Tableau Qt6 (QTreeWidget multi-colonnes)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <table> → QTreeWidget (liste plate multi-colonnes)
 * Format <input> : lignes TSV (colonnes séparées par tabulations)
 * Premier <item> = en-têtes de colonnes (séparés par |)
 * Export : ligne sélectionnée (colonnes séparées par |)
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

static void populate_from_command(QTreeWidget *tw, const char *cmd)
{
    FILE *fp = widget_opencommand(cmd);
    if (!fp) return;
    char  line[4096];
    bool  first = true;
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (!len) continue;
        QStringList cols = QString::fromUtf8(line).split('\t');
        if (first) {
            tw->setColumnCount(cols.size());
            tw->setHeaderLabels(cols);
            first = false;
        } else {
            new QTreeWidgetItem(tw, cols);
        }
    }
    fclose(fp);  /* safe_popen() uses fdopen() — pclose() is UB */
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
            if (*row) {
                QStringList c = QString::fromUtf8(row).split('|');
                if (tw->columnCount() < c.size()) tw->setColumnCount(c.size());
                new QTreeWidgetItem(tw, c);
            }
            row = attributeset_get_next(&element, Attr, ATTR_ITEM);
        }

        element = NULL;
        gchar *cmd = attributeset_get_first(&element, Attr, ATTR_INPUT);
        if (cmd && *cmd) populate_from_command(tw, cmd);
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

void widget_table_fileselect(variable *var, const char *n, const char *v) {}

void widget_table_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    QTreeWidget     *tw  = static_cast<QTreeWidget *>(var->Widget);
    QTreeWidgetItem *cur = tw->currentItem();
    if (cur) delete cur;
}

void widget_table_save(variable *var) {}
