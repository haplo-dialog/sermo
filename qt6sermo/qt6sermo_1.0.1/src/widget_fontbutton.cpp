/*
 * widget_fontbutton.cpp — Sélecteur de police Qt6 (QPushButton + QFontDialog)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <fontbutton> → FontButton : QPushButton subclass ouvrant QFontDialog au clic.
 * Export : description de la police (ex: "Sans,12,-1,5,50,0,0,0,0,0")
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
#include "widget_fontbutton.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QFontDialog>
#include <QtGui/QFont>
#include <QtCore/QString>

#include <string.h>
#include <stdlib.h>

/* ── FontButton : QPushButton qui ouvre QFontDialog au clic ─────────────── */
class FontButton : public QPushButton {
public:
    explicit FontButton(const QFont &initial, QWidget *parent = nullptr)
        : QPushButton(parent), m_font(initial)
    {
        updateLabel();
    }

    QFont currentFont() const { return m_font; }
    void  setCurrentFont(const QFont &f) { m_font = f; updateLabel(); }

protected:
    void mousePressEvent(QMouseEvent *) override {
        bool  ok;
        QFont chosen = QFontDialog::getFont(&ok, m_font, this, "Choisir une police");
        if (ok) setCurrentFont(chosen);
    }

private:
    QFont m_font;
    void updateLabel() {
        setText(m_font.family() + " " + QString::number(m_font.pointSize()));
        setProperty("currentFont", m_font.toString());
    }
};

GtkWidget *widget_fontbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QFont initial;

    if (Attr) {
        GList *element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) {
            QString qs = QString::fromUtf8(def);
            int     sp = qs.lastIndexOf(' ');
            if (sp > 0) {
                initial.setFamily(qs.left(sp));
                bool ok;
                int  sz = qs.mid(sp+1).toInt(&ok);
                if (ok && sz > 0) initial.setPointSize(sz);
            } else {
                initial.setFamily(qs);
            }
        }
    }

    FontButton *btn = new FontButton(initial);

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  btn->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) btn->setMinimumHeight(atoi(v));
    }

    return (GtkWidget *)btn;
}

gchar *widget_fontbutton_envvar_construct(GtkWidget *widget)
{
    FontButton *btn = static_cast<FontButton *>(widget);
    if (!btn) return g_strdup("");
    /* Parite gtk3/Pango : "Famille Taille" (ex. "Sans 12"), pas le format
     * interne verbeux de QFont::toString(). */
    QFont f = btn->currentFont();
    int pt = f.pointSize() > 0 ? f.pointSize() : 12;
    QString out = QString("%1 %2").arg(f.family()).arg(pt);
    return g_strdup(out.toUtf8().constData());
}

gchar *widget_fontbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_fontbutton_envvar_construct(var->Widget);
}

void widget_fontbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<FontButton *>(var->Widget)->setCurrentFont(QFont());
}

void widget_fontbutton_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_fontbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_fontbutton_removeselected(variable *var) {}
void widget_fontbutton_save(variable *var) {}
