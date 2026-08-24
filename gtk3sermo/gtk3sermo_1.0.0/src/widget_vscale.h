/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_vscale.h:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
 * Copyright (C) 2011-2012  Thunor <thunorsif@hotmail.com>
 * Copyright (C) 2026  Haplo-Linux <devel@haplo-dialog.fr> (GTK3 port)
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

#ifndef WIDGET_VSCALE_H
#define WIDGET_VSCALE_H

/* vscale delegates to widget_hscale — same GTK widget, vertical orientation */
#include "widget_hscale.h"

#define widget_vscale_clear              widget_hscale_clear
#define widget_vscale_create             widget_hscale_create
#define widget_vscale_envvar_all_construct widget_hscale_envvar_all_construct
#define widget_vscale_envvar_construct   widget_hscale_envvar_construct
#define widget_vscale_fileselect         widget_hscale_fileselect
#define widget_vscale_refresh            widget_hscale_refresh
#define widget_vscale_removeselected     widget_hscale_removeselected
#define widget_vscale_save               widget_hscale_save

#endif
