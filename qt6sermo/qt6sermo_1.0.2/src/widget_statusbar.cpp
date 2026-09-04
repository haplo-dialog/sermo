/*
 * widget_statusbar.cpp — Barre de statut Qt6 (QStatusBar)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <statusbar> → QStatusBar
 * Stocke le dernier message via showMessage().
 * Export : dernier texte affiché.
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
#include "widget_statusbar.h"

#include <QtWidgets/QStatusBar>
#include <QtWidgets/QLabel>

#include <string.h>
#include <stdlib.h>

/* QStatusBar ne mémorise pas le texte courant facilement ; on utilise
 * un QLabel intégré comme widget permanent pour le lire. */

GtkWidget *widget_statusbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QStatusBar *sb    = new QStatusBar();
    QLabel     *label = new QLabel();
    sb->addWidget(label, 1);

    if (Attr) {
        GList *element = NULL;
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) {
            label->setText(QString::fromUtf8(lbl));
            sb->showMessage(QString::fromUtf8(lbl));
        }
    }

    /* Stocker le label comme userData via setProperty */
    sb->setProperty("labelWidget", QVariant::fromValue((QObject*)label));

    return (GtkWidget *)sb;
}

gchar *widget_statusbar_envvar_construct(GtkWidget *widget)
{
    QStatusBar *sb = static_cast<QStatusBar *>(widget);
    if (!sb) return g_strdup("");
    QObject *obj = sb->property("labelWidget").value<QObject *>();
    QLabel  *lbl = qobject_cast<QLabel *>(obj);
    if (!lbl) return g_strdup("");
    return g_strdup(lbl->text().toUtf8().constData());
}

gchar *widget_statusbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_statusbar_envvar_construct(var->Widget);
}

void widget_statusbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    QStatusBar *sb = static_cast<QStatusBar *>(var->Widget);
    sb->clearMessage();
    QObject *obj = sb->property("labelWidget").value<QObject *>();
    QLabel  *lbl = qobject_cast<QLabel *>(obj);
    if (lbl) lbl->clear();
}

void widget_statusbar_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_statusbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_statusbar_removeselected(variable *var) {}
void widget_statusbar_save(variable *var) {}
