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
#include <QtWidgets/QTabBar>
#include <QtWidgets/QProxyStyle>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QStyleOptionTab>
#include <QtCore/QStringList>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ── Texte d'onglet horizontal malgré une barre verticale ─────────────────────
 * GtkNotebook ne pivote JAMAIS l'étiquette d'un onglet, même avec
 * GTK_POS_LEFT : le GtkLabel reste horizontal et c'est la colonne d'onglets
 * qui s'élargit (cf. gtk3sermo widget_notebook.c:117, gtk_label_new()).
 * Qt, lui, fait tourner le libellé de 90° pour les formes West/East, ce qui
 * rendait les onglets illisibles de travers.
 * Ce style-proxy rétablit le rendu de l'étalon : on transpose la taille
 * calculée pour l'onglet (hauteur <-> largeur) et on dessine le libellé en
 * lui présentant une forme North, donc à l'horizontale. Rien d'autre n'est
 * modifié : position, ordre, index courant et export sont inchangés. */
namespace {

static bool tab_shape_is_vertical(const QStyleOption *option)
{
    const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option);
    if (!tab) return false;
    switch (tab->shape) {
    case QTabBar::RoundedWest:
    case QTabBar::RoundedEast:
    case QTabBar::TriangularWest:
    case QTabBar::TriangularEast:
        return true;
    default:
        return false;
    }
}

class HorizontalTabTextStyle : public QProxyStyle
{
public:
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const override
    {
        QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
        if (type == QStyle::CT_TabBarTab && tab_shape_is_vertical(option))
            s.transpose();
        return s;
    }

    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override
    {
        if (element == CE_TabBarTabLabel && tab_shape_is_vertical(option)) {
            QStyleOptionTab opt(*qstyleoption_cast<const QStyleOptionTab *>(option));
            opt.shape = QTabBar::RoundedNorth;
            QProxyStyle::drawControl(element, &opt, painter, widget);
            return;
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }
};

} /* namespace anonyme */

GtkWidget *widget_notebook_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QTabWidget *tabs = new QTabWidget();

    /* Onglets à GAUCHE (colonne verticale), pour s'aligner sur le rendu de
     * gtk3dialog/gtk4dialog (référence visuelle). */
    tabs->setTabPosition(QTabWidget::West);

    /* …mais avec le LIBELLÉ à l'horizontale, comme l'étalon GTK3.
     * QWidget::setStyle() ne prend PAS possession du style : on garde une
     * instance unique et permanente (static) partagée par tous les notebooks. */
    static HorizontalTabTextStyle *horizontal_tab_style = new HorizontalTabTextStyle();
    tabs->tabBar()->setStyle(horizontal_tab_style);

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
