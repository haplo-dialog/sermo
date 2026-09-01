/* widget_password.cpp — Champ mot de passe Qt6 (QLineEdit password mode)
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
#include "widget_password.h"
#include <QtWidgets/QLineEdit>
#include <string.h>

GtkWidget *widget_password_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QLineEdit *le = new QLineEdit();
    le->setEchoMode(QLineEdit::Password);
    le->setPlaceholderText(QObject::tr("Mot de passe…"));
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) le->setText(QString::fromUtf8(def));
        if (attr) {
            const char *v;
            if ((v = get_tag_attribute(attr, "width-request"))) le->setMinimumWidth(atoi(v));
            if ((v = get_tag_attribute(attr, "tooltip"))) le->setToolTip(QString::fromUtf8(v));
        }
    }
    return (GtkWidget *)le;
}
gchar *widget_password_envvar_construct(GtkWidget *w)
{ return g_strdup(static_cast<QLineEdit*>(w)->text().toUtf8().constData()); }
gchar *widget_password_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_password_envvar_construct(v->Widget) : NULL; }
void widget_password_clear(variable *v)
{ if (v && v->Widget) static_cast<QLineEdit*>(v->Widget)->clear(); }
void widget_password_refresh(variable *v)
{ if (v && v->Widget) static_cast<QLineEdit*>(v->Widget)->update(); }
void widget_password_fileselect(variable *v, const char*, const char*) {}
void widget_password_removeselected(variable *v) {}
void widget_password_save(variable *v) {}
