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
     * sans parent (fenêtre top-level).
     * Facteur d'étirement posé comme dans widget_vbox/hbox/frame, par
     * cohérence. ⚠️ Mesuré le 2026-09-02 : avec l'enfant unique habituel d'un
     * eventbox, le rendu était DÉJÀ identique à l'étalon sans ce facteur (la
     * politique de taille Qt suffit à étendre un enfant seul). Ce n'est donc
     * pas un correctif de défaut observable, mais l'expression explicite de
     * l'intention, qui vaut dès que l'eventbox groupe plusieurs enfants. */
    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        int e = qt6_layout_get_expand((GtkWidget *)s.widgets[n]);
        layout->addWidget(static_cast<QWidget *>(s.widgets[n]), e ? 1 : 0);
    }
    return (GtkWidget *)box;
}
gchar *widget_eventbox_envvar_construct(GtkWidget *w) { (void)w; return g_strdup(""); }
gchar *widget_eventbox_envvar_all_construct(variable *var) { (void)var; return nullptr; }
void   widget_eventbox_clear(variable *var) { (void)var; }
void   widget_eventbox_refresh(variable *var) { (void)var; }
void   widget_eventbox_fileselect(variable *var, const char *n, const char *v) { (void)var;(void)n;(void)v; }
void   widget_eventbox_removeselected(variable *var) { (void)var; }
void   widget_eventbox_save(variable *var) { (void)var; }
