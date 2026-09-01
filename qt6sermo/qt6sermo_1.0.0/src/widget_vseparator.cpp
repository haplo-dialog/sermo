/*
 * widget_vseparator.cpp — Séparateur vertical Qt6 (QFrame)
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
#include "widget_vseparator.h"

#include <QtWidgets/QFrame>

#include <stdlib.h>

GtkWidget *widget_vseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QFrame *sep = new QFrame();
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  sep->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) sep->setMinimumHeight(atoi(v));
    }

    return (GtkWidget *)sep;
}

gchar *widget_vseparator_envvar_construct(GtkWidget *widget) { return NULL; }
gchar *widget_vseparator_envvar_all_construct(variable *var) { return NULL; }
void   widget_vseparator_clear(variable *var) {}
void   widget_vseparator_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget *>(var->Widget)->update(); }
void   widget_vseparator_fileselect(variable *var, const char *n, const char *v) {}
void   widget_vseparator_removeselected(variable *var) {}
void   widget_vseparator_save(variable *var) {}
