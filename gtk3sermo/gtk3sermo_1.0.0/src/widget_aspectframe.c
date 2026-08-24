/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_aspectframe.c:
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
 *
 * GtkAspectFrame — container that enforces a fixed width/height ratio.
 *
 * XML syntax:
 *   <aspectframe ratio="1.333" xalign="0.5" yalign="0.5">
 *     <image icon-name="image-x-generic" icon-size="dialog"></image>
 *   </aspectframe>
 *
 * Tag attributes:
 *   ratio="W/H"         — aspect ratio as float (default: 1.0 = square)
 *   xalign="0.0..1.0"   — horizontal alignment of child (default: 0.5)
 *   yalign="0.0..1.0"   — vertical alignment of child  (default: 0.5)
 *   obey-child="true"   — use child's own ratio instead (default: false)
 *   label="text"        — optional frame label (like <frame>)
 *
 * Exported variable: empty string (container, no value)
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

static void widget_aspectframe_input_by_command(variable *var, char *command);
static void widget_aspectframe_input_by_file(variable *var, char *filename);
static void widget_aspectframe_input_by_items(variable *var);

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_aspectframe_clear(variable *var)
{
	g_warning("%s(): Clear not implemented for this widget.", __func__);
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_aspectframe_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget   *widget;
	gchar       *sratio      = NULL;
	gchar       *sxalign     = NULL;
	gchar       *syalign     = NULL;
	gchar       *sobey       = NULL;
	gchar       *label_text  = NULL;
	gfloat       ratio       = 1.0f;
	gfloat       xalign      = 0.5f;
	gfloat       yalign      = 0.5f;
	gboolean     obey_child  = FALSE;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (attr) {
		sratio     = get_tag_attribute(attr, "ratio");
		sxalign    = get_tag_attribute(attr, "xalign");
		syalign    = get_tag_attribute(attr, "yalign");
		sobey      = get_tag_attribute(attr, "obey-child");
		label_text = get_tag_attribute(attr, "label");
	}

	/* Also check ATTR_LABEL set by <label> child tag */
	if (!label_text && attributeset_is_avail(Attr, ATTR_LABEL)) {
		GList *element;
		label_text = attributeset_get_first(&element, Attr, ATTR_LABEL);
	}

	if (sratio)  ratio      = (gfloat)g_ascii_strtod(sratio, NULL);
	if (sxalign) xalign     = (gfloat)g_ascii_strtod(sxalign, NULL);
	if (syalign) yalign     = (gfloat)g_ascii_strtod(syalign, NULL);
	if (sobey)   obey_child = (g_ascii_strcasecmp(sobey, "true") == 0 ||
	                           g_ascii_strcasecmp(sobey, "yes")  == 0 ||
	                           strcmp(sobey, "1") == 0);

	widget = gtk_aspect_frame_new(label_text, xalign, yalign, ratio, obey_child);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_aspectframe_envvar_all_construct(variable *var)
{
	return g_strdup("");
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_aspectframe_envvar_construct(GtkWidget *widget)
{
	return g_strdup("");
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_aspectframe_fileselect(
	variable *var, const char *name, const char *value)
{
	g_warning("%s(): Fileselect not implemented for this widget.", __func__);
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/

void widget_aspectframe_refresh(variable *var)
{
	GList   *element;
	gchar   *act;
	gint     initialised = FALSE;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
		initialised = GPOINTER_TO_INT(
			g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

	act = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
	while (act) {
		if (input_is_shell_command(act))
			widget_aspectframe_input_by_command(var, act + 8);
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_aspectframe_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_aspectframe_input_by_items(var);

	if (!initialised) {
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_aspectframe_removeselected(variable *var)
{
	g_warning("%s(): Removeselected not implemented for this widget.", __func__);
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_aspectframe_save(variable *var)
{
	g_warning("%s(): Save not implemented for this widget.", __func__);
}

static void widget_aspectframe_input_by_command(variable *var, char *command)
{
	g_warning("%s(): <input> not implemented for this widget.", __func__);
}

static void widget_aspectframe_input_by_file(variable *var, char *filename)
{
	g_warning("%s(): <input file> not implemented for this widget.", __func__);
}

static void widget_aspectframe_input_by_items(variable *var)
{
	g_warning("%s(): <item> not implemented for this widget.", __func__);
}
