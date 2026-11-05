/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_drawingarea.cpp — Zone de dessin Qt6 (QWidget subclasse avec paintEvent)
 * sermo — haplo-dialog — GPL-2.0-or-later */
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
#include <QtGui/QGuiApplication>
#include <string.h>

class DrawingArea : public QWidget {
public:
    explicit DrawingArea(QWidget *parent=nullptr) : QWidget(parent) {
        setMinimumSize(200,150);
        setStyleSheet("background:#1e1e2e; border:1px solid #585b70;");
    }
    /* embed="true" : une vraie fenêtre X11 à elle, où <action media> incruste
     * la vidéo (dans=$VAR) ; sans rendre natives les fenêtres qui la contiennent. */
    bool embed = false;
    void incruster() {
        embed = true;
        setAttribute(Qt::WA_DontCreateNativeAncestors);
        setAttribute(Qt::WA_NativeWindow);
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
        if ((v = get_tag_attribute(attr, "embed")) && strcasecmp(v, "true") == 0) da->incruster();
    }
    return (GtkWidget *)da;
}
/* embed="true", sous X11 (plateforme xcb) : l'identifiant X11 de la zone une
 * fois affichée ; sinon rien, comme l'étalon gtk3. */
gchar *widget_drawingarea_envvar_construct(GtkWidget *w)
{
    DrawingArea *z = static_cast<DrawingArea *>((void *)w);
    if (!z || !z->embed || !z->isVisible() || QGuiApplication::platformName() != QLatin1String("xcb"))
        return g_strdup("");
    return g_strdup_printf("%llu", (unsigned long long)z->winId());
}
gchar *widget_drawingarea_envvar_all_construct(variable *v)
{
    return v ? widget_drawingarea_envvar_construct(v->Widget) : NULL;
}
void widget_drawingarea_clear(variable *v) {}
void widget_drawingarea_refresh(variable *v)
{ if (v && v->Widget) static_cast<QWidget*>(v->Widget)->update(); }
void widget_drawingarea_fileselect(variable *v, const char*, const char*) {}
void widget_drawingarea_removeselected(variable *v) {}
void widget_drawingarea_save(variable *v) {}
