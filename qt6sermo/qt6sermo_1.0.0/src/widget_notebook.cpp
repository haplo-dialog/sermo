/*
 * widget_notebook.cpp — Onglets Qt6 (QTabWidget)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <notebook> → QTabWidget
 * Les onglets sont dépilés de la pile du parser (vbox/hbox).
 * Export : index de l'onglet courant
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
#include "stack.h"
#include "widget_notebook.h"

#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>
#include <QtCore/QStringList>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_notebook_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QTabWidget *tabs = new QTabWidget();

    /* Onglets à GAUCHE (colonne verticale), pour s'aligner sur le rendu de
     * gtk3dialog/gtk4dialog (référence visuelle). */
    tabs->setTabPosition(QTabWidget::West);

    /* Libellés d'onglets : tab-labels="A|B|C" (sinon « Tab N »). */
    QStringList tabLabels;
    if (attr) {
        const char *tl = get_tag_attribute(attr, "tab-labels");
        if (tl && *tl) tabLabels = QString::fromUtf8(tl).split('|');
    }

    /* L'instruction SUM a regroupé TOUS les onglets dans UN seul stackelement.
     * On dépile une fois et on itère s.nwidgets (widgets[0] = premier onglet).
     * L'ancienne boucle pop()-jusqu'à-NULL dépilait toute la pile → onglets
     * parasites et structure détruite. */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i) {
        if (!s.widgets[i]) continue;
        QWidget *child = static_cast<QWidget *>(s.widgets[i]);
        QString title = (i < tabLabels.size() && !tabLabels[i].isEmpty())
                        ? tabLabels[i]
                        : QString("Tab %1").arg(i + 1);
        tabs->addTab(child, title);
    }

    return (GtkWidget *)tabs;
}

gchar *widget_notebook_envvar_construct(GtkWidget *widget)
{
    QTabWidget *tabs = static_cast<QTabWidget *>(widget);
    if (!tabs) return g_strdup("0");
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", tabs->currentIndex());
    return g_strdup(buf);
}

gchar *widget_notebook_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_notebook_envvar_construct(var->Widget);
}

void widget_notebook_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QTabWidget *>(var->Widget)->setCurrentIndex(0);
}

void widget_notebook_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_notebook_fileselect(variable *var, const char *n, const char *v) {}
void widget_notebook_removeselected(variable *var) {}
void widget_notebook_save(variable *var) {}
