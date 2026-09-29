/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_drawingarea.c:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2026  haplo-dialog <devel@haplo-dialog.fr>
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
 * GtkDrawingArea — zone de dessin vierge.
 *
 * Syntaxe XML :
 *   <drawingarea width-request="300" height-request="200">
 *     <variable>CANVAS</variable>
 *   </drawingarea>
 *
 * Attributs de tag :
 *   width-request="x"   — largeur souhaitée en pixels
 *   height-request="x"  — hauteur souhaitée en pixels
 *
 * Note : aucun rendu n'est effectué par défaut ; ce widget fournit une
 * surface que des scripts externes ou de futures extensions pourront
 * peindre. Il n'exporte aucune variable.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#include "config.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "tag_attributes.h"

//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_drawingarea_clear(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif
	if (var && var->Widget)
		gtk_widget_queue_draw(var->Widget);
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_drawingarea_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget  *widget;
	gchar      *value;
	gint        width  = -1;
	gint        height = -1;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget = gtk_drawing_area_new();

	if (attr) {
		if ((value = get_tag_attribute(attr, "width-request")))
			width = atoi(value);
		if ((value = get_tag_attribute(attr, "height-request")))
			height = atoi(value);
	}

	if (width > 0 || height > 0)
		gtk_widget_set_size_request(widget, width, height);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_drawingarea_envvar_all_construct(variable *var)
{
	/* Not connected by default */
	return g_strdup("");
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_drawingarea_envvar_construct(GtkWidget *widget)
{
	/* A drawing area exports no value. */
	return g_strdup("");
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_drawingarea_fileselect(
	variable *var, const char *name, const char *value)
{
	g_warning("%s(): Fileselect not implemented for this widget.", __func__);
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/

void widget_drawingarea_refresh(variable *var)
{
	GList   *element;
	gint     initialised = FALSE;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
		initialised = GPOINTER_TO_INT(
			g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

	if (!initialised) {
		if (attributeset_is_avail(var->Attributes, ATTR_LABEL))
			g_warning("%s(): <label> not implemented for this widget.", __func__);
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);
	}

	(void) element;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_drawingarea_removeselected(variable *var)
{
	g_warning("%s(): Removeselected not implemented for this widget.", __func__);
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_drawingarea_save(variable *var)
{
	g_warning("%s(): Save not implemented for this widget.", __func__);
}
