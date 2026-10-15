/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_colorbutton.cpp — Sélecteur de couleur Qt6 (QPushButton + QColorDialog)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <colorbutton> → ColorButton : QPushButton subclass ouvrant QColorDialog au clic.
 * Export : couleur en format hexadécimal "#RRGGBB"
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
#include "widget_colorbutton.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QColorDialog>
#include <QtGui/QColor>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ── ColorButton : QPushButton qui ouvre QColorDialog au clic ────────────── */
class ColorButton : public QPushButton {
public:
    explicit ColorButton(const QColor &initial, QWidget *parent = nullptr)
        : QPushButton(parent), m_color(initial)
    {
        updateStyle();
        setFixedSize(80, 28);
    }

    QColor currentColor() const { return m_color; }
    void   setCurrentColor(const QColor &c) { m_color = c; updateStyle(); }

protected:
    /* Override mousePressEvent to open color dialog */
    void mousePressEvent(QMouseEvent *) override {
        QColor chosen = QColorDialog::getColor(m_color, this, "Choisir une couleur");
        if (chosen.isValid()) setCurrentColor(chosen);
    }

private:
    QColor m_color;
    void updateStyle() {
        setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #888; }")
                      .arg(m_color.name()));
        setProperty("currentColor", m_color.name());
    }
};

/* Couleur lue comme l'étalon gtk3sermo : la partie avant « | » (après, c'est
 * l'alpha), espaces ôtés, puis l'analyse du toolkit (« #rrggbb », « #rgb »…).
 * La forme Qt « #aarrggbb », que gdk_rgba_parse refuse, est refusée aussi.
 * Faux si illisible : rien ne change. */
static bool couleur_lire(const char *texte, QColor *couleur)
{
    if (!texte) return false;
    QString s = QString::fromUtf8(texte).section('|', 0, 0).trimmed();
    if (s.isEmpty() || (s.startsWith('#') && s.size() == 9)) return false;
    QColor c = QColor::fromString(s);
    if (!c.isValid()) return false;
    *couleur = c;
    return true;
}

GtkWidget *widget_colorbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QColor initial(Qt::white);

    if (Attr) {
        GList *element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        couleur_lire(def, &initial);
    }

    ColorButton *btn = new ColorButton(initial);

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  btn->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) btn->setMinimumHeight(atoi(v));
    }

    return (GtkWidget *)btn;
}

gchar *widget_colorbutton_envvar_construct(GtkWidget *widget)
{
    ColorButton *btn = static_cast<ColorButton *>(widget);
    if (!btn) return g_strdup("#ffffff");
    return g_strdup(btn->currentColor().name().toUtf8().constData());
}

gchar *widget_colorbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_colorbutton_envvar_construct(var->Widget);
}

void widget_colorbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<ColorButton *>(var->Widget)->setCurrentColor(QColor(Qt::white));
}

/* <input> à chaque « refresh », comme l'étalon : la première ligne est une
 * couleur. Au premier passage, un <default> lisible l'emporte (la commande
 * tourne quand même). Une ligne illisible ou vide ne change rien. Aucune
 * <action> n'est branchée sur ce widget dans ce port. Jusqu'au 2026-10-13,
 * l'<input> n'était jamais lu (CO restait #ffffff). */
void widget_colorbutton_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ColorButton *btn = static_cast<ColorButton *>(var->Widget);
    bool premier = !btn->property("sermoLu").toBool();
    btn->setProperty("sermoLu", true);
    gchar *ligne = widget_input_line(var->Attributes);
    if (!ligne) return;
    QColor c;
    if (premier) {
        GList *el = NULL;
        if (couleur_lire(attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT), &c)) {
            g_free(ligne);
            return;
        }
    }
    if (couleur_lire(ligne, &c))
        btn->setCurrentColor(c);
    g_free(ligne);
}

void widget_colorbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_colorbutton_removeselected(variable *var) {}
void widget_colorbutton_save(variable *var) {}
