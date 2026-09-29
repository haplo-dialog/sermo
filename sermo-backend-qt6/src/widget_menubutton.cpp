/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubutton.cpp — Bouton qui déroule un menu (Qt 6)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <menuitem> fabrique ici un PORTEUR invisible qui transporte le modèle en
 * propriétés Qt (menuitem_label, menuitem_action…) — même convention que
 * <menubar>. Le menu local les relit pour bâtir ses QAction.
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_menubutton.h"
#include <QtWidgets/QToolButton>
#include <QtWidgets/QMenu>
#include <QtGui/QAction>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QVariant>
#include <string.h>
#include <stdlib.h>

/* execute_action vient de actions.h : une seule déclaration, sinon violation
 * de l'ODR (type de retour et constance divergents). */
#include "actions.h"

GtkWidget *widget_menubutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;

    const char *label = nullptr;
    if (Attr) {
        GList *el = nullptr;
        gchar *l = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (l && *l) label = l;
    }
    if (!label && attr) { const char *v = get_tag_attribute(attr, "label"); if (v) label = v; }

    QToolButton *bouton = new QToolButton();
    bouton->setText(QString::fromUtf8(label ? label : "Menu"));
    bouton->setPopupMode(QToolButton::InstantPopup);

    QMenu *menu = new QMenu(bouton);
    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n) {
        QWidget *porteur = static_cast<QWidget *>(s.widgets[n]);
        if (!porteur) continue;
        QVariant kind = porteur->property("menuitem_kind");
        if (kind.toString() == QLatin1String("separator")) { menu->addSeparator(); continue; }
        QString lbl = porteur->property("menuitem_label").toString();
        QString cmd = porteur->property("menuitem_action").toString();
        QAction *a = menu->addAction(lbl);
        QObject::connect(a, &QAction::triggered, bouton, [bouton, lbl, cmd]() {
            bouton->setProperty("sermoChoix", lbl);
            if (!cmd.isEmpty()) {
                QByteArray c = cmd.toUtf8();
                execute_action((GtkWidget *) bouton, c.data(), nullptr);
            } });
    }
    bouton->setMenu(menu);
    return (GtkWidget *) bouton;
}

/* Export : le libellé du dernier élément choisi, vide avant tout choix. */
gchar *widget_menubutton_envvar_construct(GtkWidget *widget)
{
    QToolButton *b = static_cast<QToolButton *>(widget);
    if (!b) return g_strdup("");
    QByteArray t = b->property("sermoChoix").toString().toUtf8();
    return g_strdup(t.constData());
}
gchar *widget_menubutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return nullptr;
    return widget_menubutton_envvar_construct(var->Widget);
}
void widget_menubutton_clear(variable *var)
{
    if (var && var->Widget)
        static_cast<QToolButton *>(var->Widget)->setProperty("sermoChoix", QString());
}
void widget_menubutton_refresh(variable *var)        { (void) var; }
void widget_menubutton_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_menubutton_removeselected(variable *var) { (void) var; }
void widget_menubutton_save(variable *var)           { (void) var; }
