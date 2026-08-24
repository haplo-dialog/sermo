/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_image.c:
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
 * GtkImage — native GTK3 image widget (icon theme or file).
 *
 * XML syntax:
 *   <!-- From theme icon -->
 *   <image icon-name="document-save" icon-size="large"></image>
 *
 *   <!-- From file -->
 *   <image file="/path/to/image.png" width="64" height="64"></image>
 *
 *   <!-- Dynamic via <input> -->
 *   <image>
 *     <variable>MY_IMAGE</variable>
 *     <input>echo /path/to/image.png</input>
 *   </image>
 *
 * Tag attributes:
 *   icon-name="name"          — XDG icon name from current theme
 *   icon-size="button|small|large|dialog"  (default: large = 32px)
 *   file="/path/to/file"      — PNG, SVG, JPG, BMP ...
 *   width="N"                 — scale width  (with file=)
 *   height="N"                — scale height (with file=)
 *
 * Exported variable:
 *   MY_IMAGE = icon-name or file path currently displayed
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

static void widget_image_input_by_command(variable *var, char *command);
static void widget_image_input_by_file(variable *var, char *filename);
static void widget_image_input_by_items(variable *var);

/* Helper: map icon-size string → GtkIconSize */
static GtkIconSize parse_icon_size(const gchar *s)
{
	if (!s) return GTK_ICON_SIZE_LARGE_TOOLBAR;
	if (g_ascii_strcasecmp(s, "button")  == 0) return GTK_ICON_SIZE_BUTTON;
	if (g_ascii_strcasecmp(s, "small")   == 0) return GTK_ICON_SIZE_SMALL_TOOLBAR;
	if (g_ascii_strcasecmp(s, "large")   == 0) return GTK_ICON_SIZE_LARGE_TOOLBAR;
	if (g_ascii_strcasecmp(s, "dnd")     == 0) return GTK_ICON_SIZE_DND;
	if (g_ascii_strcasecmp(s, "dialog")  == 0) return GTK_ICON_SIZE_DIALOG;
	return GTK_ICON_SIZE_LARGE_TOOLBAR;
}

/* Helper: set image from path, with optional scaling */
static void image_set_from_path(GtkWidget *widget, const gchar *path,
                                 gint req_w, gint req_h)
{
	GdkPixbuf *pb;
	GError    *err = NULL;

	pb = gdk_pixbuf_new_from_file(path, &err);
	if (!pb) {
		g_warning("widget_image: cannot load '%s': %s", path,
		          err ? err->message : "unknown error");
		if (err) g_error_free(err);
		gtk_image_set_from_icon_name(GTK_IMAGE(widget),
		    "image-missing", GTK_ICON_SIZE_LARGE_TOOLBAR);
		return;
	}

	if (req_w > 0 || req_h > 0) {
		gint orig_w = gdk_pixbuf_get_width(pb);
		gint orig_h = gdk_pixbuf_get_height(pb);
		gint dst_w  = (req_w > 0) ? req_w : orig_w;
		gint dst_h  = (req_h > 0) ? req_h : orig_h;
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pb, dst_w, dst_h,
		                        GDK_INTERP_BILINEAR);
		g_object_unref(pb);
		pb = scaled;
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(widget), pb);
	g_object_unref(pb);
}

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_image_clear(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif
	gtk_image_clear(GTK_IMAGE(var->Widget));
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_image_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget   *widget;
	gchar       *icon_name = NULL;
	gchar       *file      = NULL;
	gchar       *sizestr   = NULL;
	gchar       *swidth    = NULL;
	gchar       *sheight   = NULL;
	GtkIconSize  icon_size;
	gint         req_w = 0, req_h = 0;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget = gtk_image_new();

	if (attr) {
		icon_name = get_tag_attribute(attr, "icon-name");
		file      = get_tag_attribute(attr, "file");
		sizestr   = get_tag_attribute(attr, "icon-size");
		swidth    = get_tag_attribute(attr, "width");
		sheight   = get_tag_attribute(attr, "height");
	}

	if (swidth)  req_w = atoi(swidth);
	if (sheight) req_h = atoi(sheight);
	icon_size = parse_icon_size(sizestr);

	if (icon_name) {
		gtk_image_set_from_icon_name(GTK_IMAGE(widget), icon_name, icon_size);
	} else if (file) {
		image_set_from_path(widget, file, req_w, req_h);
	} else {
		/* Placeholder until <input> fills it */
		gtk_image_set_from_icon_name(GTK_IMAGE(widget),
		    "image-missing", GTK_ICON_SIZE_LARGE_TOOLBAR);
	}

	/* Store scale hints for later refresh */
	g_object_set_data(G_OBJECT(widget), "_img_w", GINT_TO_POINTER(req_w));
	g_object_set_data(G_OBJECT(widget), "_img_h", GINT_TO_POINTER(req_h));
	g_object_set_data(G_OBJECT(widget), "_img_icon_size",
	    GINT_TO_POINTER((gint)icon_size));

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_image_envvar_all_construct(variable *var)
{
	return g_strdup("");
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_image_envvar_construct(GtkWidget *widget)
{
	const gchar *icon_name = NULL;
	GtkIconSize  icon_size;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	gtk_image_get_icon_name(GTK_IMAGE(widget), &icon_name, &icon_size);
	if (icon_name)
		return g_strdup(icon_name);

	/* If loaded from pixbuf we don't have the original path — return "" */
	return g_strdup("");
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_image_fileselect(
	variable *var, const char *name, const char *value)
{
	g_warning("%s(): Fileselect not implemented for this widget.", __func__);
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/

void widget_image_refresh(variable *var)
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
			widget_image_input_by_command(var, act + 8);
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_image_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_image_input_by_items(var);

	if (!initialised) {
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
			gchar *def = attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT);
			if (def && *def) {
				gint w = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(var->Widget), "_img_w"));
				gint h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(var->Widget), "_img_h"));
				image_set_from_path(var->Widget, def, w, h);
			}
		}
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

void widget_image_removeselected(variable *var)
{
	g_warning("%s(): Removeselected not implemented for this widget.", __func__);
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_image_save(variable *var)
{
	g_warning("%s(): Save not implemented for this widget.", __func__);
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_image_input_by_command(variable *var, char *command)
{
	FILE  *infile;
	gchar  line[1024];
	gint   count;
	gint   w, h;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = widget_opencommand(command))) {
		if (fgets(line, sizeof(line), infile)) {
			line[sizeof(line) - 1] = 0;
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			w = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(var->Widget), "_img_w"));
			h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(var->Widget), "_img_h"));
			if (line[0] == '/')
				image_set_from_path(var->Widget, line, w, h);
			else
				gtk_image_set_from_icon_name(GTK_IMAGE(var->Widget), line,
				    (GtkIconSize)GPOINTER_TO_INT(
				        g_object_get_data(G_OBJECT(var->Widget), "_img_icon_size")));
		}
		fclose(infile);
	} else {
		g_warning("%s(): Couldn't open '%s' for reading.", __func__, command);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by File                                                       *
 ***********************************************************************/

static void widget_image_input_by_file(variable *var, char *filename)
{
	FILE  *infile;
	gchar  line[1024];
	gint   count;
	gint   w, h;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = fopen(filename, "r"))) {
		if (fgets(line, sizeof(line), infile)) {
			line[sizeof(line) - 1] = 0;
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			w = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(var->Widget), "_img_w"));
			h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(var->Widget), "_img_h"));
			if (line[0] == '/')
				image_set_from_path(var->Widget, line, w, h);
			else
				gtk_image_set_from_icon_name(GTK_IMAGE(var->Widget), line,
				    (GtkIconSize)GPOINTER_TO_INT(
				        g_object_get_data(G_OBJECT(var->Widget), "_img_icon_size")));
		}
		fclose(infile);
	} else {
		g_warning("%s(): Couldn't open '%s' for reading.", __func__, filename);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Items                                                      *
 ***********************************************************************/

static void widget_image_input_by_items(variable *var)
{
	g_warning("%s(): <item> not implemented for this widget.", __func__);
}
