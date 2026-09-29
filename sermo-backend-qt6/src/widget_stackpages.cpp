/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_stackpages.cpp — N pages, une seule visible (Qt 6)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * QStackedWidget est l'équivalent exact de GtkStack. Qt n'a pas de
 * « switcher » tout fait : la rangée de boutons est construite à la main, un
 * bouton par page, chacun poussant sa page.
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_stackpages.h"
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_stackpages_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    int page = 0, avec_switcher = 0;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "page"))) page = atoi(v);
        if ((v = get_tag_attribute(attr, "switcher")))
            avec_switcher = (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || atoi(v) == 1);
    }

    QStackedWidget *pile = new QStackedWidget();
    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n)
        if (s.widgets[n]) pile->addWidget(static_cast<QWidget *>(s.widgets[n]));

    if (page < 0) page = 0;
    if (page >= pile->count() && pile->count() > 0) page = pile->count() - 1;
    pile->setCurrentIndex(page);

    if (!avec_switcher) return (GtkWidget *) pile;

    /* Enveloppe : la rangée de boutons au-dessus, la pile dessous. */
    QWidget     *hote   = new QWidget();
    QVBoxLayout *vertic = new QVBoxLayout(hote);
    QWidget     *barre  = new QWidget();
    QHBoxLayout *horiz  = new QHBoxLayout(barre);
    vertic->setContentsMargins(0, 0, 0, 0);
    horiz->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < pile->count(); ++i) {
        QPushButton *b = new QPushButton(QString::number(i + 1));
        b->setCheckable(true);
        b->setChecked(i == page);
        QObject::connect(b, &QPushButton::clicked, pile, [pile, i]() { pile->setCurrentIndex(i); });
        horiz->addWidget(b);
    }
    horiz->addStretch(1);
    vertic->addWidget(barre);
    vertic->addWidget(pile);
    hote->setProperty("sermoPile", QVariant::fromValue<void *>(pile));
    return (GtkWidget *) hote;
}

/* Export : l'index de la page visible, comme <notebook> (étalon gtk3). */
gchar *widget_stackpages_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("0");
    QWidget *w = static_cast<QWidget *>(widget);
    QStackedWidget *pile = qobject_cast<QStackedWidget *>(w);
    if (!pile) {
        /* Avec un switcher, c'est l'enveloppe qui est rendue : la pile y est
         * accrochée en propriété. */
        QVariant v = w->property("sermoPile");
        if (v.isValid()) pile = static_cast<QStackedWidget *>(v.value<void *>());
    }
    if (!pile) return g_strdup("0");
    QByteArray t = QString::number(pile->currentIndex()).toUtf8();
    return g_strdup(t.constData());
}
gchar *widget_stackpages_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return nullptr;
    return widget_stackpages_envvar_construct(var->Widget);
}
void widget_stackpages_clear(variable *var)          { (void) var; }
void widget_stackpages_refresh(variable *var)        { (void) var; }
void widget_stackpages_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_stackpages_removeselected(variable *var) { (void) var; }
void widget_stackpages_save(variable *var)           { (void) var; }
