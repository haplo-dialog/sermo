/*
 * widget_vbox.cpp — Conteneur vertical Qt6 (QWidget + QVBoxLayout)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "stack.h"
#include "widget_vbox.h"

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>

#include <stdlib.h>

GtkWidget *widget_vbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int spacing = 2;
    if (attr) {
        const char *v = get_tag_attribute(attr, "spacing");
        if (v) spacing = atoi(v);
    }

    QWidget     *box    = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(spacing);

    /* Le parser émet push(new vbox(pop())) : l'instruction SUM a regroupé
     * TOUS les enfants frères dans UN seul stackelement (s.nwidgets). On
     * dépile une fois et on les ré-parente dans le layout (sinon ils restent
     * sans parent → chacun devient une fenêtre top-level). */
    stackelement s = pop();
    bool anyExpand = false;
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        int e = qt6_layout_get_expand((GtkWidget *)s.widgets[n]);
        if (e) anyExpand = true;
        layout->addWidget(static_cast<QWidget *>(s.widgets[n]), e ? 1 : 0);
    }
    /* Aucun enfant extensible → ressort final pour empiler en haut (sinon Qt
     * répartit l'espace vertical à parts égales et espace trop les lignes). */
    if (!anyExpand) layout->addStretch(1);

    return (GtkWidget *)box;
}

gchar *widget_vbox_envvar_construct(GtkWidget *widget) { return NULL; }
gchar *widget_vbox_envvar_all_construct(variable *var) { return NULL; }
void   widget_vbox_clear(variable *var) {}
void   widget_vbox_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget *>(var->Widget)->update(); }
void   widget_vbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_vbox_removeselected(variable *var) {}
void   widget_vbox_save(variable *var) {}
