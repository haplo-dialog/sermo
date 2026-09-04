/*
 * widget_colorbutton.cpp — Sélecteur de couleur Qt6 (QPushButton + QColorDialog)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
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

GtkWidget *widget_colorbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QColor initial(Qt::white);

    if (Attr) {
        GList *element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) {
            QColor c(QString::fromUtf8(def));
            if (c.isValid()) initial = c;
        }
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

void widget_colorbutton_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_colorbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_colorbutton_removeselected(variable *var) {}
void widget_colorbutton_save(variable *var) {}
