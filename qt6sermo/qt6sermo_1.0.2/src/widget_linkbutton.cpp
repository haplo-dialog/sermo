/* widget_linkbutton.cpp — Bouton lien Qt6 (QPushButton → ouvre l'URL). */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_linkbutton.h"
#include <QtWidgets/QPushButton>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>

GtkWidget *widget_linkbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void)attr; (void)Type;
    GList *el = nullptr;
    const char *label = nullptr, *uri = nullptr;
    if (Attr) {
        gchar *l = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (l && *l) label = l;
        el = nullptr;
        gchar *in = attributeset_get_first(&el, Attr, ATTR_INPUT);
        if (in && *in) uri = in;
    }
    QString url = (uri && *uri) ? QString::fromUtf8(uri)
                                : QString::fromUtf8(label ? label : "");
    QPushButton *btn = new QPushButton(QString::fromUtf8(label && *label ? label : (uri ? uri : "")));
    QObject::connect(btn, &QPushButton::clicked, btn,
                     [url]() { if (!url.isEmpty()) QDesktopServices::openUrl(QUrl(url)); });
    return (GtkWidget *)btn;
}
gchar *widget_linkbutton_envvar_construct(GtkWidget *w)
{ QPushButton *b = static_cast<QPushButton*>(w); return g_strdup(b ? b->text().toUtf8().constData() : ""); }
gchar *widget_linkbutton_envvar_all_construct(variable *var) { if(!var||!var->Widget) return nullptr; return widget_linkbutton_envvar_construct(var->Widget); }
void   widget_linkbutton_clear(variable *var) { (void)var; }
void   widget_linkbutton_refresh(variable *var) { (void)var; }
void   widget_linkbutton_fileselect(variable *var, const char *n, const char *v) { (void)var;(void)n;(void)v; }
void   widget_linkbutton_removeselected(variable *var) { (void)var; }
void   widget_linkbutton_save(variable *var) { (void)var; }
