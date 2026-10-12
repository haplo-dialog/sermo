/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_vbox.cpp — Conteneur vertical Qt6 (QWidget + QVBoxLayout)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
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

    /* Rangement des enfants (étirement, homogénéité, calage) : règles de
     * gtk3, communes aux deux boîtes — voir qt6_box_construire(), widgets.cpp. */
    QWidget *box = qt6_box_construire(attr, false, spacing);

    return (GtkWidget *)box;
}

gchar *widget_vbox_envvar_construct(GtkWidget *widget) { return NULL; }
gchar *widget_vbox_envvar_all_construct(variable *var) { return NULL; }
void   widget_vbox_clear(variable *var) {}
void   widget_vbox_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget *>(var->Widget)->update(); }
void   widget_vbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_vbox_removeselected(variable *var) {}
void   widget_vbox_save(variable *var) {}
