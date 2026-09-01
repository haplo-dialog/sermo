/* widget_aspectframe.cpp — Cadre à rapport d'aspect fixe Qt6
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
#include "widget_aspectframe.h"
#include <QtWidgets/QFrame>
#include <QtWidgets/QVBoxLayout>
#include <string.h>

GtkWidget *widget_aspectframe_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QFrame *frame = new QFrame();
    frame->setFrameShape(QFrame::Box);
    frame->setFrameShadow(QFrame::Sunken);
    new QVBoxLayout(frame);
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "label"))) frame->setToolTip(QString::fromUtf8(v));
        if ((v = get_tag_attribute(attr, "width-request")))  frame->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) frame->setMinimumHeight(atoi(v));
    }
    return (GtkWidget *)frame;
}
gchar *widget_aspectframe_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_aspectframe_envvar_all_construct(variable *v) { return NULL; }
void widget_aspectframe_clear(variable *v) {}
void widget_aspectframe_refresh(variable *v)
{ if (v && v->Widget) static_cast<QFrame*>(v->Widget)->update(); }
void widget_aspectframe_fileselect(variable *v, const char*, const char*) {}
void widget_aspectframe_removeselected(variable *v) {}
void widget_aspectframe_save(variable *v) {}
