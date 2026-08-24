/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_vseparator.c — GtkSeparator vertical (GTK4)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * XML : <vseparator/>
 *
 * Note : GtkVSeparator a été supprimé en GTK4.
 * On utilise gtk_separator_new(GTK_ORIENTATION_VERTICAL).
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#include "config.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_vseparator.h"

void widget_vseparator_clear(variable *var) {}

GtkWidget *widget_vseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    return gtk_separator_new(GTK_ORIENTATION_VERTICAL);
}

gchar *widget_vseparator_envvar_all_construct(variable *var) { return NULL; }
gchar *widget_vseparator_envvar_construct(GtkWidget *widget) { return g_strdup(""); }
void   widget_vseparator_fileselect(variable *var, const char *name, const char *value) {}
void   widget_vseparator_refresh(variable *var) {}
void   widget_vseparator_removeselected(variable *var) {}
void   widget_vseparator_save(variable *var) {}
