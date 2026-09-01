/*
 * widget_hseparator.cpp — Séparateur horizontal Qt6 (QFrame)
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
#include "widget_hseparator.h"

#include <QtWidgets/QFrame>

#include <stdlib.h>

GtkWidget *widget_hseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QFrame *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  sep->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) sep->setMinimumHeight(atoi(v));
    }

    return (GtkWidget *)sep;
}

gchar *widget_hseparator_envvar_construct(GtkWidget *widget) { return NULL; }
gchar *widget_hseparator_envvar_all_construct(variable *var) { return NULL; }
void   widget_hseparator_clear(variable *var) {}
void   widget_hseparator_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget *>(var->Widget)->update(); }
void   widget_hseparator_fileselect(variable *var, const char *n, const char *v) {}
void   widget_hseparator_removeselected(variable *var) {}
void   widget_hseparator_save(variable *var) {}
