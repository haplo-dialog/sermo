/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_colorbutton.c: 
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  L�szl� Pere <pipas@linux.pte.hu>
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

/* Includes */
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

/* Defines */
//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

/* Local function prototypes, located at file bottom */
static void widget_colorbutton_input_by_command(variable *var, char *command);
static void widget_colorbutton_input_by_file(variable *var, char *filename);
static void widget_colorbutton_input_by_items(variable *var);

/* Notes: */

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_colorbutton_clear(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning( "%s(): Clear not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/
GtkWidget *widget_colorbutton_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget        *widget;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget = gtk_color_button_new();

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_colorbutton_envvar_all_construct(variable *var)
{
	gchar            *string;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* This function should not be connected-up by default */

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return NULL;  /* stub: aucune variable agrégée à exporter */
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_colorbutton_envvar_construct(GtkWidget *widget)
{
	GdkRGBA          color;
	gchar             envvar[32];
	gchar            *string;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(GTK_COLOR_BUTTON(widget)), &color);

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s(): color.red  =#%04x  rgb=%02x\n", __func__,
		color.red, (guint8)(color.red * 255.0 + 0.5));
	fprintf(stderr, "%s(): color.green=#%04x  rgb=%02x\n", __func__,
		color.green, (guint8)(color.green * 255.0 + 0.5));
	fprintf(stderr, "%s(): color.blue =#%04x  rgb=%02x\n", __func__,
		color.blue, (guint8)(color.blue * 255.0 + 0.5));
#endif

	if (gtk_color_button_get_use_alpha(GTK_COLOR_BUTTON(widget))) {
		g_snprintf(envvar, sizeof(envvar), "#%02x%02x%02x|%u",
			(guint8)(color.red * 255.0 + 0.5),
			(guint8)(color.green * 255.0 + 0.5),
			(guint8)(color.blue * 255.0 + 0.5),
			gtk_color_button_get_alpha(GTK_COLOR_BUTTON(widget)));

	} else {
		g_snprintf(envvar, sizeof(envvar), "#%02x%02x%02x",
			(guint8)(color.red * 255.0 + 0.5),
			(guint8)(color.green * 255.0 + 0.5),
			(guint8)(color.blue * 255.0 + 0.5));
	}

	string = g_strdup(envvar);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_colorbutton_fileselect(
	variable *var, const char *name, const char *value)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning( "%s(): Fileselect not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/
void widget_colorbutton_refresh(variable *var)
{
	GdkRGBA          color;
	GList            *element;
	gchar            *act;
	gint              initialised = FALSE;
	guint             alpha;
	list_t           *values = NULL;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Get initialised state of widget */
	if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
		initialised = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

	/* The <input> tag... */
	act = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
	while (act) {
		if (input_is_shell_command(act))
			widget_colorbutton_input_by_command(var, act + 8);
		/* input file stock = "File:", input file = "File:/path/to/file" */
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised) {
				/* Check for file-monitor and create if requested */
				widget_file_monitor_try_create(var, act + 5);
			}
			widget_colorbutton_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_colorbutton_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
		if (attributeset_is_avail(var->Attributes, ATTR_LABEL))
			g_warning( "%s(): <label> not implemented for this widget.",
				__func__);

		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
			values = linecutter(g_strdup(attributeset_get_first(&element,
				var->Attributes, ATTR_DEFAULT)), '|');
			if (values->n_lines > 0) {
				/* Parse the RGB value to create the necessary GdkRGBA.
				 * This function doesn't like trailing whitespace so it
				 * needs to be stripped first with g_strstrip() */ 
				if (gdk_rgba_parse(&color, g_strstrip(values->line[0]))) {
#ifdef DEBUG_CONTENT
					fprintf(stderr, "%s:() valid colour found\n", __func__);
#endif
					gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(GTK_COLOR_BUTTON(var->Widget)), &color);
				}
			}
			if (values->n_lines > 1) {
				/* Read alpha as an unsigned decimal integer */
				if (sscanf(values->line[1], "%u", &alpha) == 1) {
#ifdef DEBUG_CONTENT
					fprintf(stderr, "%s:() valid alpha=%u found\n", __func__, alpha);
#endif
					/* This requires use-alpha="true" */
					gtk_color_button_set_alpha(GTK_COLOR_BUTTON(var->Widget), alpha);
				}
			}
			/* Free linecutter memory */
			if (values) list_t_free(values);
		}

		if (attributeset_is_avail(var->Attributes, ATTR_HEIGHT))
			g_warning( "%s(): <height> not implemented for this widget.",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_WIDTH))
			g_warning( "%s(): <width> not implemented for this widget.",
				__func__);
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||	/* Deprecated */
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);

		/* Connect signals */
		g_signal_connect(G_OBJECT(var->Widget), "color-set",
			G_CALLBACK(on_any_widget_color_set_event), (gpointer)var->Attributes);

	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_colorbutton_removeselected(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning( "%s(): Removeselected not implemented for this widget.",
		__func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_colorbutton_save(variable *var)
{
	FILE             *outfile;
	GdkRGBA          color;
	GList            *element;
	gchar            *act;
	gchar            *filename = NULL;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* We'll use the output file filename if available */
	act = attributeset_get_first(&element, var->Attributes, ATTR_OUTPUT);
	while (act) {
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			filename = act + 5;
			break;
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_OUTPUT);
	}

	/* If we have a valid filename then open it and dump the
	 * widget's data to it */
	if (filename) {
		if ((outfile = fopen(filename, "w"))) {
			gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(GTK_COLOR_BUTTON(var->Widget)), &color);
			if (gtk_color_button_get_use_alpha(GTK_COLOR_BUTTON(var->Widget))) {
				fprintf(outfile, "#%02x%02x%02x|%u",
					(guint8)(color.red * 255.0 + 0.5),
					(guint8)(color.green * 255.0 + 0.5),
					(guint8)(color.blue * 255.0 + 0.5),
					gtk_color_button_get_alpha(GTK_COLOR_BUTTON(var->Widget)));

			} else {
				fprintf(outfile, "#%02x%02x%02x",
					(guint8)(color.red * 255.0 + 0.5),
					(guint8)(color.green * 255.0 + 0.5),
					(guint8)(color.blue * 255.0 + 0.5));
			}
			/* Close the file */
			fclose(outfile);
		} else {
			g_warning( "%s(): Couldn't open '%s' for writing.",
				__func__, filename);
		}
	} else {
		g_warning( "%s(): No <output file> directive found.", __func__);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_colorbutton_input_by_command(variable *var, char *command)
{
	FILE             *infile;
	GdkRGBA          color;
	list_t           *values = NULL;
	gchar             line[512];
	gint              count;
	guint             alpha;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s(): command: '%s'\n", __func__, command);
#endif

	/* Opening pipe for reading... */
	if ((infile = widget_opencommand(command))) {
		/* Just one line */
		if (fgets(line, 512, infile)) {
			/* Enforce end of string in case of max chars read */
			line[512 - 1] = 0;
			/* Remove the trailing [CR]LFs */
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			values = linecutter(g_strdup(line), '|');
			if (values->n_lines > 0) {
				/* Parse the RGB value to create the necessary GdkRGBA.
				 * This function doesn't like trailing whitespace so it
				 * needs to be stripped first with g_strstrip() */ 
				if (gdk_rgba_parse(&color, g_strstrip(values->line[0]))) {
#ifdef DEBUG_CONTENT
					fprintf(stderr, "%s:() valid colour found\n", __func__);
#endif
					gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(GTK_COLOR_BUTTON(var->Widget)), &color);
				}
			}
			if (values->n_lines > 1) {
				/* Read alpha as an unsigned decimal integer */
				if (sscanf(values->line[1], "%u", &alpha) == 1) {
#ifdef DEBUG_CONTENT
					fprintf(stderr, "%s:() valid alpha=%u found\n", __func__, alpha);
#endif
					/* This requires use-alpha="true" */
					gtk_color_button_set_alpha(GTK_COLOR_BUTTON(var->Widget), alpha);
				}
			}
			/* Free linecutter memory */
			if (values) list_t_free(values);
		}
		/* Close the file */
		fclose(infile);
	} else {
		g_warning( "%s(): Couldn't open '%s' for reading.", __func__,
			command);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by File                                                       *
 ***********************************************************************/

static void widget_colorbutton_input_by_file(variable *var, char *filename)
{
	FILE             *infile;
	GdkRGBA          color;
	list_t           *values = NULL;
	gchar             line[512];
	gint              count;
	guint             alpha;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = fopen(filename, "r"))) {
		/* Just one line */
		if (fgets(line, 512, infile)) {
			/* Enforce end of string in case of max chars read */
			line[512 - 1] = 0;
			/* Remove the trailing [CR]LFs */
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			values = linecutter(g_strdup(line), '|');
			if (values->n_lines > 0) {
				/* Parse the RGB value to create the necessary GdkRGBA.
				 * This function doesn't like trailing whitespace so it
				 * needs to be stripped first with g_strstrip() */ 
				if (gdk_rgba_parse(&color, g_strstrip(values->line[0]))) {
#ifdef DEBUG_CONTENT
					fprintf(stderr, "%s:() valid colour found\n", __func__);
#endif
					gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(GTK_COLOR_BUTTON(var->Widget)), &color);
				}
			}
			if (values->n_lines > 1) {
				/* Read alpha as an unsigned decimal integer */
				if (sscanf(values->line[1], "%u", &alpha) == 1) {
#ifdef DEBUG_CONTENT
					fprintf(stderr, "%s:() valid alpha=%u found\n", __func__, alpha);
#endif
					/* This requires use-alpha="true" */
					gtk_color_button_set_alpha(GTK_COLOR_BUTTON(var->Widget), alpha);
				}
			}
			/* Free linecutter memory */
			if (values) list_t_free(values);
		}
		/* Close the file */
		fclose(infile);
	} else {
		g_warning( "%s(): Couldn't open '%s' for reading.", __func__,
			filename);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Items                                                      *
 ***********************************************************************/

static void widget_colorbutton_input_by_items(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning( "%s(): <item> not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}
