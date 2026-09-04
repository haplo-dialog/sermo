/*
 * widgets.h: Widget manipulation functions
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
 * Copyright (C) 2011-2012  Thunor <thunorsif@hotmail.com>
 * Copyright (C) 2026  haplo-dialog <devel@haplo-dialog.fr> (GTK3 port, security)
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

#ifndef WIDGETS_H
#define WIDGETS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#ifndef QT6_COMPAT_H
#include <gtk/gtk.h>
#endif
#include "gtk3d.h"
#include "stack.h"
#include "attributes.h"
#include "stringman.h"
#include "variables.h"
#include "automaton.h"

#ifdef __cplusplus
extern "C" {
#endif

char *widget_get_text_value(GtkWidget *widget, int type);
FILE *widget_opencommand(const char *command);
double widget_command_value(AttributeSet *Attr, double fallback);
char *widgets_to_str(int itype);
gboolean widget_connect_signals(GtkWidget *widget, AttributeSet *Attr);
void widget_visibility_list_add(GtkWidget *widget, tag_attr *attr);
void widget_show_all(void);
/* Disposition type GTK : enregistre space-expand de chaque widget à sa création
 * (hook unique dans l'automate) ; les boîtes interrogent l'extensibilité de
 * leurs enfants pour appliquer les bons facteurs d'étirement Qt. */
void qt6_layout_register(GtkWidget *widget, tag_attr *attr);
int  qt6_layout_get_expand(GtkWidget *widget);

#ifdef __cplusplus
}
#endif

#endif
