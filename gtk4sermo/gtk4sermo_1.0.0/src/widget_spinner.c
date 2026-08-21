/*
 * widget_spinner.c:
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
 * GtkSpinner — animated activity indicator.
 *
 * XML syntax:
 *   <spinner>
 *     <variable>MY_SPINNER</variable>
 *     <input>echo 1</input>         <!-- 1 = active, 0 = stopped -->
 *   </spinner>
 *
 *   <spinner active="true"></spinner>
 *
 * Tag attributes:
 *   active="true|false"   — initial state (default: false)
 *
 * Actions:
 *   <action>start:MY_SPINNER</action>   — start spinning
 *   <action>stop:MY_SPINNER</action>    — stop spinning
 *   <action>enable:MY_SPINNER</action>
 *   <action>disable:MY_SPINNER</action>
 *
 * Exported variable:
 *   MY_SPINNER=1  (active) or  MY_SPINNER=0  (inactive)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#include "config.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "tag_attributes.h"

//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

static void widget_spinner_input_by_command(variable *var, char *command);
static void widget_spinner_input_by_file(variable *var, char *filename);
static void widget_spinner_input_by_items(variable *var);

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_spinner_clear(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif
	gtk_spinner_stop(GTK_SPINNER(var->Widget));
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_spinner_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget  *widget;
	gchar      *value;
	gboolean    active = FALSE;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget = gtk_spinner_new();

	if (attr) {
		if ((value = get_tag_attribute(attr, "active"))) {
			if (g_ascii_strcasecmp(value, "true") == 0 ||
			    g_ascii_strcasecmp(value, "yes")  == 0 ||
			    strcmp(value, "1") == 0)
				active = TRUE;
		}
	}

	if (active)
		gtk_spinner_start(GTK_SPINNER(widget));

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_spinner_envvar_all_construct(variable *var)
{
	/* Not connected by default */
	return g_strdup("");
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_spinner_envvar_construct(GtkWidget *widget)
{
	gboolean spinning;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_object_get(G_OBJECT(widget), "active", &spinning, NULL);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
	return g_strdup(spinning ? "1" : "0");
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_spinner_fileselect(
	variable *var, const char *name, const char *value)
{
	g_warning("%s(): Fileselect not implemented for this widget.", __func__);
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/

void widget_spinner_refresh(variable *var)
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

	/* <input> — "1" or non-zero → start; "0" → stop */
	act = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
	while (act) {
		if (input_is_shell_command(act))
			widget_spinner_input_by_command(var, act + 8);
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_spinner_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_spinner_input_by_items(var);

	if (!initialised) {
		if (attributeset_is_avail(var->Attributes, ATTR_LABEL))
			g_warning("%s(): <label> not implemented for this widget.", __func__);
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
			gchar *def = attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT);
			if (def && (atoi(def) != 0 ||
			    g_ascii_strcasecmp(def, "true") == 0 ||
			    g_ascii_strcasecmp(def, "yes") == 0))
				gtk_spinner_start(GTK_SPINNER(var->Widget));
			else
				gtk_spinner_stop(GTK_SPINNER(var->Widget));
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

void widget_spinner_removeselected(variable *var)
{
	g_warning("%s(): Removeselected not implemented for this widget.", __func__);
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_spinner_save(variable *var)
{
	g_warning("%s(): Save not implemented for this widget.", __func__);
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_spinner_input_by_command(variable *var, char *command)
{
	FILE  *infile;
	gchar  line[64];
	gint   count;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = widget_opencommand(command))) {
		if (fgets(line, sizeof(line), infile)) {
			line[sizeof(line) - 1] = 0;
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			if (atoi(line) != 0 ||
			    g_ascii_strcasecmp(line, "true") == 0 ||
			    g_ascii_strcasecmp(line, "yes") == 0)
				gtk_spinner_start(GTK_SPINNER(var->Widget));
			else
				gtk_spinner_stop(GTK_SPINNER(var->Widget));
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

static void widget_spinner_input_by_file(variable *var, char *filename)
{
	FILE  *infile;
	gchar  line[64];
	gint   count;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = fopen(filename, "r"))) {
		if (fgets(line, sizeof(line), infile)) {
			line[sizeof(line) - 1] = 0;
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			if (atoi(line) != 0 ||
			    g_ascii_strcasecmp(line, "true") == 0 ||
			    g_ascii_strcasecmp(line, "yes") == 0)
				gtk_spinner_start(GTK_SPINNER(var->Widget));
			else
				gtk_spinner_stop(GTK_SPINNER(var->Widget));
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

static void widget_spinner_input_by_items(variable *var)
{
	g_warning("%s(): <item> not implemented for this widget.", __func__);
}
