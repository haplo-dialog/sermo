/* widget_infobar.cpp — Barre d'information Qt6 (QFrame coloré + QLabel)
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
#include "widget_infobar.h"
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <string.h>

GtkWidget *widget_infobar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QFrame *frame = new QFrame();
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setStyleSheet("QFrame { background:#313244; border-left:4px solid #89b4fa; padding:4px; }");
    QHBoxLayout *lay = new QHBoxLayout(frame);
    lay->setContentsMargins(8,4,8,4);
    QLabel *icon = new QLabel("ℹ");
    icon->setStyleSheet("color:#89b4fa; font-size:16px; font-weight:bold;");
    QLabel *lbl  = new QLabel();
    lbl->setObjectName("ib_label");
    lbl->setWordWrap(true);
    lbl->setStyleSheet("color:#cdd6f4;");
    lay->addWidget(icon);
    lay->addWidget(lbl, 1);

    if (Attr) {
        GList *el = NULL;
        gchar *msg = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (!msg) msg = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (msg && *msg) lbl->setText(QString::fromUtf8(msg));
        /* type=info|warning|error */
        if (attr) {
            const char *v = get_tag_attribute(attr, "message-type");
            if (v) {
                if (strcmp(v,"warning")==0)
                    frame->setStyleSheet("QFrame{background:#2a2a1e;border-left:4px solid #f9e2af;padding:4px;}");
                else if (strcmp(v,"error")==0)
                    frame->setStyleSheet("QFrame{background:#2a1e1e;border-left:4px solid #f38ba8;padding:4px;}");
            }
        }
    }
    return (GtkWidget *)frame;
}
gchar *widget_infobar_envvar_construct(GtkWidget *w)
{
    /* Parité gtk3sermo : la variable d'un <infobar> rend le texte de son label. */
    QWidget *frame = static_cast<QWidget *>(w);
    if (!frame) return g_strdup("");
    QLabel *lbl = frame->findChild<QLabel *>("ib_label");
    return g_strdup(lbl ? lbl->text().toUtf8().constData() : "");
}
gchar *widget_infobar_envvar_all_construct(variable *v) { return NULL; }
void widget_infobar_clear(variable *v) {}
void widget_infobar_refresh(variable *v)
{ if (v && v->Widget) static_cast<QFrame*>(v->Widget)->update(); }
void widget_infobar_fileselect(variable *v, const char*, const char*) {}
void widget_infobar_removeselected(variable *v) {}
void widget_infobar_save(variable *v) {}
