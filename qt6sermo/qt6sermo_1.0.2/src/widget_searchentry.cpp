/* widget_searchentry.cpp — Champ de recherche Qt6 (QLineEdit + icône loupe)
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
#include "widget_searchentry.h"
#include <QtWidgets/QLineEdit>
#include <string.h>

GtkWidget *widget_searchentry_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QLineEdit *le = new QLineEdit();
    le->setPlaceholderText(QObject::tr("Rechercher…"));
    le->addAction(QIcon::fromTheme("edit-find"), QLineEdit::LeadingPosition);
    le->setClearButtonEnabled(true);
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) le->setText(QString::fromUtf8(def));
        if (attr) {
            const char *v;
            if ((v = get_tag_attribute(attr, "width-request"))) le->setMinimumWidth(atoi(v));
        }
    }
    return (GtkWidget *)le;
}
gchar *widget_searchentry_envvar_construct(GtkWidget *w)
{ return g_strdup(static_cast<QLineEdit*>(w)->text().toUtf8().constData()); }
gchar *widget_searchentry_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_searchentry_envvar_construct(v->Widget) : NULL; }
void widget_searchentry_clear(variable *v)
{ if (v && v->Widget) static_cast<QLineEdit*>(v->Widget)->clear(); }
void widget_searchentry_refresh(variable *v)
{ if (v && v->Widget) static_cast<QLineEdit*>(v->Widget)->update(); }
void widget_searchentry_fileselect(variable *v, const char*, const char*) {}
void widget_searchentry_removeselected(variable *v) {}
void widget_searchentry_save(variable *v) {}
