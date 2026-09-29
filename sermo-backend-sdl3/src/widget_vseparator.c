/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_vseparator.h"

GtkWidget *widget_vseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    return (GtkWidget *)widget_node_new(WT_SEPARATOR, NULL, "");
}
gchar *widget_vseparator_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_vseparator_envvar_all_construct(variable *var) { return NULL; }
void   widget_vseparator_clear(variable *var) {}
void   widget_vseparator_refresh(variable *var) {}
void   widget_vseparator_fileselect(variable *var, const char *n, const char *v) {}
void   widget_vseparator_removeselected(variable *var) {}
void   widget_vseparator_save(variable *var) {}
