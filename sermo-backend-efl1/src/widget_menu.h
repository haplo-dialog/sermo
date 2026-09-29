/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_menu.h — efl1sermo GPL-2.0-or-later */
#ifndef WIDGET_MENU_H
#define WIDGET_MENU_H
#include "gtkdialog.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "variables.h"
GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar *widget_menu_envvar_construct(GtkWidget *w);
gchar *widget_menu_envvar_all_construct(variable *var);
void   widget_menu_clear(variable *var);
void   widget_menu_refresh(variable *var);
void   widget_menu_fileselect(variable *var, const char *n, const char *v);
void   widget_menu_removeselected(variable *var);
void   widget_menu_save(variable *var);
#endif
