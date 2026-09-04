/* widget_drawingarea.cpp — Zone de dessin Qt6 (QWidget subclasse avec paintEvent)
 * qt6sermo 1.0.0 — haplo-dialog — GPL-2.0-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_drawingarea.h"
#include <QtWidgets/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QColor>
#include <string.h>

class DrawingArea : public QWidget {
public:
    explicit DrawingArea(QWidget *parent=nullptr) : QWidget(parent) {
        setMinimumSize(200,150);
        setStyleSheet("background:#1e1e2e; border:1px solid #585b70;");
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        /* Zone vide — les scripts shells ne peuvent pas dessiner directement */
        p.setPen(QColor("#585b70"));
        p.drawText(rect(), Qt::AlignCenter, "[drawing area]");
    }
};

GtkWidget *widget_drawingarea_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    DrawingArea *da = new DrawingArea();
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  da->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) da->setMinimumHeight(atoi(v));
    }
    return (GtkWidget *)da;
}
gchar *widget_drawingarea_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_drawingarea_envvar_all_construct(variable *v) { return NULL; }
void widget_drawingarea_clear(variable *v) {}
void widget_drawingarea_refresh(variable *v)
{ if (v && v->Widget) static_cast<QWidget*>(v->Widget)->update(); }
void widget_drawingarea_fileselect(variable *v, const char*, const char*) {}
void widget_drawingarea_removeselected(variable *v) {}
void widget_drawingarea_save(variable *v) {}
