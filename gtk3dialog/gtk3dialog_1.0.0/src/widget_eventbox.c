/*
 * widget_eventbox.c: 
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
static void widget_eventbox_input_by_command(variable *var, char *command);
static void widget_eventbox_input_by_file(variable *var, char *filename);
static void widget_eventbox_input_by_items(variable *var);

/* Notes: */

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_eventbox_clear(variable *var)
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
GtkWidget *widget_eventbox_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget        *widget;
	stackelement      s;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Create the eventbox widget */
	widget = gtk_event_box_new();

	/* Pop the widgets that the eventbox will contain and add them */
	s = pop();
	gtk_container_add(GTK_CONTAINER(widget), s.widgets[0]);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_eventbox_envvar_all_construct(variable *var)
{
	gchar            *string;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* This function should not be connected-up by default */

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s(): Hello.\n", __func__);
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return NULL;  /* stub: aucune variable agrégée à exporter */
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_eventbox_envvar_construct(GtkWidget *widget)
{
	gchar            *string;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	string = g_strdup("");

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_eventbox_fileselect(
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
void widget_eventbox_refresh(variable *var)
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
			widget_eventbox_input_by_command(var, act + 8);
		/* input file stock = "File:", input file = "File:/path/to/file" */
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5)
			widget_eventbox_input_by_file(var, act + 5);
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_eventbox_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
		if (attributeset_is_avail(var->Attributes, ATTR_LABEL))
			g_warning( "%s(): <label> not implemented for this widget.",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT))
			g_warning( "%s(): <default> not implemented for this widget.",
				__func__);
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

	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_eventbox_removeselected(variable *var)
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

void widget_eventbox_save(variable *var)
{
	FILE             *outfile;
	GList            *element;
	gboolean          above_child;
	gboolean          visible_window;
	gchar            *act;
	gchar            *filename = NULL;
	gchar             string[32];

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

#if 0	/* Pointless since loading doesn't work at run-time */
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

			above_child = gtk_event_box_get_above_child(GTK_EVENT_BOX(var->Widget));
			if (above_child) {
				g_strlcpy(string, "true", sizeof(string));
			} else {
				g_strlcpy(string, "false", sizeof(string));
			}
			visible_window = gtk_event_box_get_visible_window(GTK_EVENT_BOX(var->Widget));
			if (visible_window) {
				g_strlcat(string, "|true", sizeof(string));
			} else {
				g_strlcat(string, "|false", sizeof(string));
			}
			fprintf(outfile, "%s", string);

			/* Close the file */
			fclose(outfile);
		} else {
			g_warning("%s(): Couldn't open '%s' for writing.", __func__, filename);
		}
	} else {
		g_warning("%s(): No <output file> directive found.", __func__);
	}
#endif

	g_warning( "%s(): Save not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_eventbox_input_by_command(variable *var, char *command)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning( "%s(): <input> not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by File                                                       *
 ***********************************************************************/

static void widget_eventbox_input_by_file(variable *var, char *filename)
{
	FILE             *infile;
	gchar             line[512];
	gint              count;
	list_t           *sliced;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

#if 0	/* At run-time this has no effect -- shame */
	if (infile = fopen(filename, "r")) {
		/* Just one line */
		if (fgets(line, 512, infile)) {
			/* Enforce end of string in case of max chars read */
			line[512 - 1] = 0;
			/* Remove the trailing [CR]LFs */
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;

			sliced = linecutter(g_strdup(line), '|');
#ifdef DEBUG_CONTENT
			fprintf(stderr, "%s(): line=%s\n", __func__, line);
			fprintf(stderr, "%s(): sliced->n_lines=%i\n", __func__,
				sliced->n_lines);
			fprintf(stderr, "%s(): sliced->line[0]=%s\n", __func__,
				sliced->line[0]);
			fprintf(stderr, "%s(): sliced->line[1]=%s\n", __func__,
				sliced->line[1]);
#endif
			for (count = 0; count < sliced->n_lines; count++) {
				if ((strcasecmp(sliced->line[count], "true") == 0) ||
					(strcasecmp(sliced->line[count], "yes") == 0) ||
					(atoi(sliced->line[count]) == 1)) {
					if (count == 0) {
#ifdef DEBUG_CONTENT
fprintf(stderr, "%s(): setting count=%i TRUE\n", __func__, count);
#endif
						gtk_event_box_set_above_child(GTK_EVENT_BOX(
							var->Widget), TRUE);
					} else {
#ifdef DEBUG_CONTENT
fprintf(stderr, "%s(): setting count=%i TRUE\n", __func__, count);
#endif
						gtk_event_box_set_visible_window(GTK_EVENT_BOX(
							var->Widget), TRUE);
					}
				} else if ((strcasecmp(sliced->line[count], "false") == 0) ||
					(strcasecmp(sliced->line[count], "no") == 0) ||
					(strcasecmp(sliced->line[count], "0") == 0)) {
					if (count == 0) {
#ifdef DEBUG_CONTENT
fprintf(stderr, "%s(): setting count=%i FALSE\n", __func__, count);
#endif
						gtk_event_box_set_above_child(GTK_EVENT_BOX(
							var->Widget), FALSE);
					} else {
#ifdef DEBUG_CONTENT
fprintf(stderr, "%s(): setting count=%i FALSE\n", __func__, count);
#endif
						gtk_event_box_set_visible_window(GTK_EVENT_BOX(
							var->Widget), FALSE);
					}
				}
			}
			if (sliced) list_t_free(sliced);	/* Free linecutter memory */
		}

		/* Close the file */
		fclose(infile);
	} else {
		g_warning("%s(): Couldn't open '%s' for reading.", __func__, filename);
	}
#endif

	g_warning( "%s(): <input file> not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Items                                                      *
 ***********************************************************************/

static void widget_eventbox_input_by_items(variable *var)
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
