/*
 * widget_spinbutton.c: 
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  L�szl� Pere <pipas@linux.pte.hu>
 * Copyright (C) 2011-2012  Thunor <thunorsif@hotmail.com>
 * Copyright (C) 2026  Haplo-Linux <devel@haplo-dialog.fr> (GTK3 port, security)
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
#include "tag_attributes.h"

/* Defines */
//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

/* Local function prototypes, located at file bottom */
static void widget_spinbutton_input_by_command(variable *var, char *command);
static void widget_spinbutton_input_by_file(variable *var, char *filename);
static void widget_spinbutton_input_by_items(variable *var);

/* Notes: */

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_spinbutton_clear(variable *var)
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
GtkWidget *widget_spinbutton_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget        *widget;
	gdouble           range_min = 0;
	gdouble           range_max = 100;
	gdouble           range_step = 1;
	gdouble           range_value = 0;
	gchar            *value;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* These "range-*" names are consistent with the h/vscale widgets */
	if (attr) {
		if ((value = get_tag_attribute(attr, "range-min")))
			range_min = g_ascii_strtod(value, NULL);
		if ((value = get_tag_attribute(attr, "range-max")))
			range_max = g_ascii_strtod(value, NULL);
		if ((value = get_tag_attribute(attr, "range-step")))
			range_step = g_ascii_strtod(value, NULL);
		if ((value = get_tag_attribute(attr, "range-value")))
			range_value = g_ascii_strtod(value, NULL);
	}

	widget = gtk_spin_button_new_with_range(range_min, range_max, range_step);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), range_value);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_spinbutton_envvar_all_construct(variable *var)
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

gchar *widget_spinbutton_envvar_construct(GtkWidget *widget)
{
	gchar             envvar[32];
	gchar            *string;
	gdouble           value;
	guint             digits;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	digits = gtk_spin_button_get_digits(GTK_SPIN_BUTTON(widget));
	value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	switch (digits) {
		case 0:
			g_snprintf(envvar, sizeof(envvar), "%.0f", value);
			break;
		case 1:
			g_snprintf(envvar, sizeof(envvar), "%.1f", value);
			break;
		case 2:
			g_snprintf(envvar, sizeof(envvar), "%.2f", value);
			break;
		case 3:
			g_snprintf(envvar, sizeof(envvar), "%.3f", value);
			break;
		case 4:
			g_snprintf(envvar, sizeof(envvar), "%.4f", value);
			break;
		case 5:
			g_snprintf(envvar, sizeof(envvar), "%.5f", value);
			break;
		case 6:
			g_snprintf(envvar, sizeof(envvar), "%.6f", value);
			break;
		case 7:
			g_snprintf(envvar, sizeof(envvar), "%.7f", value);
			break;
		case 8:
			g_snprintf(envvar, sizeof(envvar), "%.8f", value);
			break;
		case 9:
			g_snprintf(envvar, sizeof(envvar), "%.9f", value);
			break;
		case 10:
			g_snprintf(envvar, sizeof(envvar), "%.10f", value);
			break;
		case 11:
			g_snprintf(envvar, sizeof(envvar), "%.11f", value);
			break;
		case 12:
			g_snprintf(envvar, sizeof(envvar), "%.12f", value);
			break;
		case 13:
			g_snprintf(envvar, sizeof(envvar), "%.13f", value);
			break;
		case 14:
			g_snprintf(envvar, sizeof(envvar), "%.14f", value);
			break;
		case 15:
			g_snprintf(envvar, sizeof(envvar), "%.15f", value);
			break;
		case 16:
			g_snprintf(envvar, sizeof(envvar), "%.16f", value);
			break;
		/* Is there much point going beyond 16? */
		default:
			g_snprintf(envvar, sizeof(envvar), "%f", value);
			break;
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

void widget_spinbutton_fileselect(
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
void widget_spinbutton_refresh(variable *var)
{
	GList            *element;
	gchar            *act;
	gint              initialised = FALSE;

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
			widget_spinbutton_input_by_command(var, act + 8);
		/* input file stock = "File:", input file = "File:/path/to/file" */
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised) {
				/* Check for file-monitor and create if requested */
				widget_file_monitor_try_create(var, act + 5);
			}
			widget_spinbutton_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_spinbutton_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
		if (attributeset_is_avail(var->Attributes, ATTR_LABEL))
			g_warning( "%s(): <label> not implemented for this widget.",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT))
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(var->Widget),
				g_ascii_strtod(attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT), NULL));
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
		g_signal_connect(G_OBJECT(var->Widget), "value-changed",
			G_CALLBACK(on_any_widget_value_changed_event), (gpointer)var->Attributes);
		g_signal_connect(G_OBJECT(var->Widget), "changed", 
			G_CALLBACK(on_any_widget_changed_event), (gpointer)var->Attributes);
		g_signal_connect(G_OBJECT(var->Widget), "activate",
			G_CALLBACK(on_any_widget_activate_event), (gpointer)var->Attributes);
/* GTK3: always available - removed version guard */
		/* Despite what the GTK+ 2 Reference Manual says, I found
		 * these to be activatable by default. They will actually
		 * be prefixed with either primary- or secondary- for use
		 * within action directives */
		g_signal_connect(G_OBJECT(var->Widget), "icon-press",
			G_CALLBACK(on_any_widget_icon_press_event), (gpointer)var->Attributes);
		g_signal_connect(G_OBJECT(var->Widget), "icon-release",
			G_CALLBACK(on_any_widget_icon_release_event), (gpointer)var->Attributes);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_spinbutton_removeselected(variable *var)
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

void widget_spinbutton_save(variable *var)
{
	FILE             *outfile;
	GList            *element;
	gchar            *act;
	gchar            *filename = NULL;
	gdouble           value;
	guint             digits;

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
			digits = gtk_spin_button_get_digits(GTK_SPIN_BUTTON(var->Widget));
			value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(var->Widget));
			switch (digits) {
				case 0:
					fprintf(outfile, "%.0f", value);
					break;
				case 1:
					fprintf(outfile, "%.1f", value);
					break;
				case 2:
					fprintf(outfile, "%.2f", value);
					break;
				case 3:
					fprintf(outfile, "%.3f", value);
					break;
				case 4:
					fprintf(outfile, "%.4f", value);
					break;
				case 5:
					fprintf(outfile, "%.5f", value);
					break;
				case 6:
					fprintf(outfile, "%.6f", value);
					break;
				case 7:
					fprintf(outfile, "%.7f", value);
					break;
				case 8:
					fprintf(outfile, "%.8f", value);
					break;
				case 9:
					fprintf(outfile, "%.9f", value);
					break;
				case 10:
					fprintf(outfile, "%.10f", value);
					break;
				case 11:
					fprintf(outfile, "%.11f", value);
					break;
				case 12:
					fprintf(outfile, "%.12f", value);
					break;
				case 13:
					fprintf(outfile, "%.13f", value);
					break;
				case 14:
					fprintf(outfile, "%.14f", value);
					break;
				case 15:
					fprintf(outfile, "%.15f", value);
					break;
				case 16:
					fprintf(outfile, "%.16f", value);
					break;
				/* Is there much point going beyond 16? */
				default:
					fprintf(outfile, "%f", value);
					break;
			}
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

static void widget_spinbutton_input_by_command(variable *var, char *command)
{
	FILE             *infile;
	gchar             line[512];
	gint              count;

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
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(var->Widget), g_ascii_strtod(line, NULL));
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

static void widget_spinbutton_input_by_file(variable *var, char *filename)
{
	FILE             *infile;
	gchar             line[512];
	gint              count;

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
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(var->Widget), g_ascii_strtod(line, NULL));
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

static void widget_spinbutton_input_by_items(variable *var)
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
