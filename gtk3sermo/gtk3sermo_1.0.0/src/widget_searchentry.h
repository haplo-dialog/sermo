/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_searchentry.h:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2026  Haplo-Linux <devel@haplo-dialog.fr> (GTK3 new widget)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef WIDGET_SEARCHENTRY_H
#define WIDGET_SEARCHENTRY_H

/* Function prototypes */
void widget_searchentry_clear(variable *var);
GtkWidget *widget_searchentry_create(
	AttributeSet *Attr, tag_attr *attr, gint Type);
gchar *widget_searchentry_envvar_all_construct(variable *var);
gchar *widget_searchentry_envvar_construct(GtkWidget *widget);
void widget_searchentry_fileselect(
	variable *var, const char *name, const char *value);
void widget_searchentry_refresh(variable *var);
void widget_searchentry_removeselected(variable *var);
void widget_searchentry_save(variable *var);

#endif
