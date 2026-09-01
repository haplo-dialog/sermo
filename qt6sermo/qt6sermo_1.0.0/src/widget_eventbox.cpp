/* widget_eventbox.cpp — Conteneur Qt6 (QWidget) qui héberge son enfant. */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_eventbox.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>

GtkWidget *widget_eventbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void)Attr; (void)attr; (void)Type;
    QWidget     *box    = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    /* Dépiler l'enfant (groupé par SUM) et le ré-parenter — sinon il reste
     * sans parent (fenêtre top-level). */
    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n)
        if (s.widgets[n]) layout->addWidget(static_cast<QWidget *>(s.widgets[n]));
    return (GtkWidget *)box;
}
gchar *widget_eventbox_envvar_construct(GtkWidget *w) { (void)w; return g_strdup(""); }
gchar *widget_eventbox_envvar_all_construct(variable *var) { (void)var; return nullptr; }
void   widget_eventbox_clear(variable *var) { (void)var; }
void   widget_eventbox_refresh(variable *var) { (void)var; }
void   widget_eventbox_fileselect(variable *var, const char *n, const char *v) { (void)var;(void)n;(void)v; }
void   widget_eventbox_removeselected(variable *var) { (void)var; }
void   widget_eventbox_save(variable *var) { (void)var; }
