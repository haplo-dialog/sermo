/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * glade_support.c: The interface between Glade and Gtkdialog.
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
 * Copyright (C) 2011-2012  Thunor <thunorsif@hotmail.com>
 * Copyright (C) 2026  haplo-dialog <devel@haplo-dialog.fr> (SDL3 port)
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
 *
 * ─── Port SDL3 ────────────────────────────────────────────────────────────
 * Le support Glade/GtkBuilder reposait entièrement sur GTK (GtkBuilder,
 * connexion de signaux GObject, introspection de types GtkWidget). Le port
 * SDL3/ImGui n'a pas d'équivalent : la description de l'interface se fait
 * uniquement via le format XML natif de gtkdialog. Glade est donc désactivé
 * (HAVE_GLADE_LIB == 0) et ce module se réduit à un avertissement.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "gtkdialog.h"
#include "glade_support.h"

/***********************************************************************
 * Run Program By Glade — non supporté dans le port SDL3               *
 ***********************************************************************/

void run_program_by_glade(const gchar *filename, const gchar *window_name)
{
	(void) window_name;
	g_warning("%s(): Glade/GtkBuilder support is not available in the SDL3 "
		"port (file '%s' ignored). Use the native gtkdialog XML format.",
		__func__, filename ? filename : "(null)");
}
