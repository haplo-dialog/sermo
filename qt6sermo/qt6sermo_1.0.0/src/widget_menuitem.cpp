/*
 * widget_menuitem.cpp — Éléments de menu Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Modèle aligné sur gtk3dialog (la référence) :
 *   <menuitem>          → élément cliquable (label + action)
 *   <menuitemseparator> → séparateur
 *   <menu>              → QMenu (déroulant) contenant les éléments/sous-menus
 *   <menubar>           → QMenuBar (cf. widget_menubar.cpp)
 *
 * CONTRAINTE : sur ce port, TOUT ce qui transite par la pile doit être un
 * QWidget (push_widget, layouts, show_all… le supposent). Or QAction N'EST
 * PAS un QWidget. On représente donc chaque <menuitem>/<menuitemseparator>
 * par un QWidget « porteur » invisible qui stocke label/action/genre dans des
 * propriétés Qt. widget_menu_create() lit ces propriétés pour fabriquer les
 * vrais QAction du QMenu, puis détruit les porteurs. Les sous-menus (<menu>)
 * sont des QMenu — donc déjà des QWidget — et restent tels quels sur la pile.
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
#include "actions.h"
#include "widget_menuitem.h"
#include "widget_menubar.h"   /* déclaration C de widget_menu_create */

#include <QtWidgets/QMenu>
#include <QtWidgets/QWidget>
#include <QtGui/QAction>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <string.h>
#include <stdlib.h>

/* ── <menuitem> / <menuitemseparator> → QWidget porteur ──────────────────── */
GtkWidget *widget_menuitem_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QWidget *holder = new QWidget();   /* porteur invisible (respecte l'invariant) */
    holder->setVisible(false);

    if (Type == WIDGET_MENUITEMSEPARATOR) {
        holder->setProperty("menuitem_kind", QStringLiteral("separator"));
        return (GtkWidget *)holder;
    }

    /* Label : attribut de balise « label="…" » prioritaire, sinon <label>. */
    const char *label = "menuitem";
    const char *tv = attr ? get_tag_attribute(attr, "label") : nullptr;
    if (tv && *tv) {
        label = tv;
    } else if (Attr) {
        GList *element = nullptr;
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) label = lbl;
    }
    holder->setProperty("menuitem_kind",  QStringLiteral("item"));
    holder->setProperty("menuitem_label", QString::fromUtf8(label));

    if (Attr) {
        GList *element = nullptr;
        gchar *cmd = attributeset_get_first(&element, Attr, ATTR_ACTION);
        if (cmd && *cmd)
            holder->setProperty("menuitem_action", QString::fromUtf8(cmd));
    }

    /* checkbox="true|false" / radiobutton="true|false" : item cochable, la
     * valeur donne aussi l'etat initial (parite gtk3sermo). */
    if (attr) {
        const char *cb = get_tag_attribute(attr, "checkbox");
        const char *rb = get_tag_attribute(attr, "radiobutton");
        const char *st = cb ? cb : rb;
        if (st) {
            holder->setProperty("menuitem_check",
                cb ? QStringLiteral("check") : QStringLiteral("radio"));
            holder->setProperty("menuitem_checked",
                (strcasecmp(st, "true") == 0 || strcmp(st, "1") == 0));
        }
    }

    return (GtkWidget *)holder;
}

gchar *widget_menuitem_envvar_construct(GtkWidget *widget)
{
    /* Un menuitem cochable rend "true"/"false" ; sinon rien. Le porteur, garde
     * vivant et parente au QMenu, porte le QAction reel via une propriete. */
    QWidget *holder = static_cast<QWidget *>(widget);
    if (!holder) return g_strdup("");
    QVariant a = holder->property("menuitem_qaction");
    if (!a.isValid()) return g_strdup("");
    QAction *act = qobject_cast<QAction *>(a.value<QObject *>());
    if (!act || !act->isCheckable()) return g_strdup("");
    return g_strdup(act->isChecked() ? "true" : "false");
}
gchar *widget_menuitem_envvar_all_construct(variable *var) { return NULL; }
void   widget_menuitem_clear(variable *var) {}
void   widget_menuitem_refresh(variable *var) {}
void   widget_menuitem_fileselect(variable *var, const char *n, const char *v) {}
void   widget_menuitem_removeselected(variable *var) {}
void   widget_menuitem_save(variable *var) {}

/* ── <menu> → QMenu (déroulant) ──────────────────────────────────────────── */
GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    /* Label : attribut de balise « label="…" » prioritaire, sinon <label>. */
    const char *label = "menu";
    const char *tv = attr ? get_tag_attribute(attr, "label") : nullptr;
    if (tv && *tv) {
        label = tv;
    } else if (Attr) {
        GList *element = nullptr;
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) label = lbl;
    }

    QMenu *menu = new QMenu(QString::fromUtf8(label));

    /* Enfants regroupés (SUM) en UN stackelement : on dépile UNE fois. */
    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        QObject *o = static_cast<QObject *>(s.widgets[n]);

        /* Sous-menu : un QMenu est déjà un QWidget. */
        if (QMenu *sub = qobject_cast<QMenu *>(o)) {
            menu->addMenu(sub);
            continue;
        }

        /* Porteur de menuitem : on en extrait le vrai QAction. */
        QWidget *holder = qobject_cast<QWidget *>(o);
        if (!holder) continue;

        QString kind = holder->property("menuitem_kind").toString();
        if (kind == QLatin1String("separator")) {
            menu->addSeparator();
        } else {
            QString lbl = holder->property("menuitem_label").toString();
            QAction *act = menu->addAction(lbl);
            QVariant a = holder->property("menuitem_action");
            if (a.isValid()) {
                QString qcmd = a.toString();
                QObject::connect(act, &QAction::triggered, [qcmd]() {
                    execute_action(nullptr, qcmd.toUtf8().constData(), nullptr);
                });
            }
            QString chk = holder->property("menuitem_check").toString();
            if (!chk.isEmpty()) {
                act->setCheckable(true);
                act->setChecked(holder->property("menuitem_checked").toBool());
                /* Le porteur (QWidget) reste la valeur exportee : garde vivant,
                 * parente au menu, porteur du QAction reel. */
                holder->setProperty("menuitem_qaction",
                                    QVariant::fromValue((QObject *)act));
                holder->setParent(menu);
                holder->hide();
                continue;   /* NE PAS detruire ce porteur */
            }
        }
        holder->deleteLater();   /* porteur consommé */
    }

    return (GtkWidget *)menu;
}

gchar *widget_menu_envvar_construct(GtkWidget *widget) { return NULL; }
gchar *widget_menu_envvar_all_construct(variable *var) { return NULL; }
void   widget_menu_clear(variable *var) {}
void   widget_menu_refresh(variable *var) {}
void   widget_menu_fileselect(variable *var, const char *n, const char *v) {}
void   widget_menu_removeselected(variable *var) {}
void   widget_menu_save(variable *var) {}
