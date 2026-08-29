/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_infobar.h:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2026  haplo-dialog <devel@haplo-dialog.fr> (GTK4 port)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef WIDGET_INFOBAR_H
#define WIDGET_INFOBAR_H

/* Function prototypes */
void widget_infobar_clear(variable *var);
GtkWidget *widget_infobar_create(
	AttributeSet *Attr, tag_attr *attr, gint Type);
gchar *widget_infobar_envvar_all_construct(variable *var);
gchar *widget_infobar_envvar_construct(GtkWidget *widget);
void widget_infobar_fileselect(
	variable *var, const char *name, const char *value);
void widget_infobar_refresh(variable *var);
void widget_infobar_removeselected(variable *var);
void widget_infobar_save(variable *var);

#endif
