/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_wizard.cpp — Suite d'étapes (Qt 6)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ PAS QWizard : c'est une FENÊTRE (QDialog), pas un conteneur qu'on pose
 * dans une <vbox>. L'assistant est donc composé — QStackedWidget plus trois
 * boutons — exactement comme sur les six autres ports.
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_wizard.h"
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QVariant>
#include <string.h>
#include <stdlib.h>

/* execute_action vient de actions.h : une seule déclaration, sinon violation
 * de l'ODR (type de retour et constance divergents). */
#include "actions.h"

GtkWidget *widget_wizard_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) attr; (void) Type;

    QStackedWidget *pile = new QStackedWidget();
    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n)
        if (s.widgets[n]) pile->addWidget(static_cast<QWidget *>(s.widgets[n]));

    QWidget     *hote   = new QWidget();
    QVBoxLayout *vertic = new QVBoxLayout(hote);
    QWidget     *barre  = new QWidget();
    QHBoxLayout *horiz  = new QHBoxLayout(barre);
    vertic->setContentsMargins(0, 0, 0, 0);
    horiz->setContentsMargins(0, 0, 0, 0);

    QPushButton *prec = new QPushButton(QString::fromUtf8("Précédent"));
    QPushButton *suiv = new QPushButton(QString::fromUtf8("Suivant"));
    QPushButton *fin  = new QPushButton(QString::fromUtf8("Terminer"));
    horiz->addWidget(prec); horiz->addWidget(suiv); horiz->addWidget(fin);
    horiz->addStretch(1);

    QObject::connect(prec, &QPushButton::clicked, pile, [pile]() {
        int i = pile->currentIndex() - 1; if (i < 0) i = 0; pile->setCurrentIndex(i); });
    QObject::connect(suiv, &QPushButton::clicked, pile, [pile]() {
        int i = pile->currentIndex() + 1;
        if (i >= pile->count()) i = pile->count() ? pile->count() - 1 : 0;
        pile->setCurrentIndex(i); });
    /* « Terminer » joue l'<action> du wizard ; il ne ferme rien de lui-même. */
    QObject::connect(fin, &QPushButton::clicked, fin, [Attr, fin]() {
        if (!Attr) return;
        GList *el = nullptr;
        gchar *cmd = attributeset_get_first(&el, Attr, ATTR_ACTION);
        while (cmd) {
            if (*cmd) execute_action((GtkWidget *) fin, cmd, nullptr);
            cmd = attributeset_get_next(&el, Attr, ATTR_ACTION);
        } });

    vertic->addWidget(pile);
    vertic->addWidget(barre);
    hote->setProperty("sermoPile", QVariant::fromValue<void *>(pile));
    return (GtkWidget *) hote;
}

/* Export : l'index de l'étape courante, comme <stack>. */
gchar *widget_wizard_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("0");
    QWidget *w = static_cast<QWidget *>(widget);
    QVariant v = w->property("sermoPile");
    QStackedWidget *pile = v.isValid() ? static_cast<QStackedWidget *>(v.value<void *>()) : nullptr;
    if (!pile) return g_strdup("0");
    QByteArray t = QString::number(pile->currentIndex()).toUtf8();
    return g_strdup(t.constData());
}
gchar *widget_wizard_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return nullptr;
    return widget_wizard_envvar_construct(var->Widget);
}
void widget_wizard_clear(variable *var)          { (void) var; }
void widget_wizard_refresh(variable *var)        { (void) var; }
void widget_wizard_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_wizard_removeselected(variable *var) { (void) var; }
void widget_wizard_save(variable *var)           { (void) var; }
