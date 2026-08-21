/*
 * widget_pulse.h:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2026  Haplo-Linux <devel@haplo-dialog.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef WIDGET_PULSE_H
#define WIDGET_PULSE_H

void      widget_pulse_clear(variable *var);
GtkWidget *widget_pulse_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar    *widget_pulse_envvar_all_construct(variable *var);
gchar    *widget_pulse_envvar_construct(GtkWidget *widget);
void      widget_pulse_fileselect(variable *var, const char *name, const char *value);
void      widget_pulse_refresh(variable *var);
void      widget_pulse_removeselected(variable *var);
void      widget_pulse_save(variable *var);

#endif
