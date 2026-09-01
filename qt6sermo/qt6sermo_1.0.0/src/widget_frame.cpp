/*
 * widget_frame.cpp — Cadre avec label Qt6 (QGroupBox)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <frame> → QGroupBox non checkable avec titre
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
#include "widget_frame.h"

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_frame_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *label = "";
    if (Attr) {
        GList *element = NULL;
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) label = lbl;
    }

    QGroupBox   *gb     = new QGroupBox(QString::fromUtf8(label));
    QVBoxLayout *layout = new QVBoxLayout(gb);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(2);
    gb->setCheckable(false);

    /* Dépiler les enfants groupés par SUM et les ré-parenter (cf. vbox). */
    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n)
        if (s.widgets[n])
            layout->addWidget(static_cast<QWidget *>(s.widgets[n]));

    return (GtkWidget *)gb;
}

gchar *widget_frame_envvar_construct(GtkWidget *widget) { return NULL; }
gchar *widget_frame_envvar_all_construct(variable *var) { return NULL; }
void   widget_frame_clear(variable *var) {}
void   widget_frame_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget *>(var->Widget)->update(); }
void   widget_frame_fileselect(variable *var, const char *n, const char *v) {}
void   widget_frame_removeselected(variable *var) {}
void   widget_frame_save(variable *var) {}
