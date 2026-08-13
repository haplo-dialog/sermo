/*
 * widget_window.c: 
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

extern gboolean option_centering;
extern gboolean have_geometry_xy;
extern gboolean have_geometry_dxdy;
extern gint geometry_dx;
extern gint geometry_dy;
extern gint geometry_x;
extern gint geometry_y;

/* Local function prototypes, located at file bottom */
static void widget_window_input_by_command(variable *var, char *command);
static void widget_window_input_by_file(variable *var, char *filename);
static void widget_window_input_by_items(variable *var);

/***********************************************************************
 * Auto-size on realize                                                *
 *                                                                     *
 * Fired once when the window is realized (before it appears).        *
 * Algorithm:                                                          *
 *   1. Ask GTK for the natural (preferred) size of the packed content.*
 *   2. If the caller already set a default size (via default-width /  *
 *      default-height attributes) respect it, but clamp it down when  *
 *      it exceeds the monitor work area.                              *
 *   3. If no default size was set, use the natural content size,      *
 *      again clamped to 95 % of the monitor work area.               *
 *                                                                     *
 * Result: the window always fits on screen and is never bigger than   *
 * its content requires.                                               *
 ***********************************************************************/
static void widget_window_on_realize(GtkWidget *widget, gpointer data)
{
	GdkDisplay    *display;
	GdkMonitor    *monitor;
	GdkRectangle   workarea;
	GtkRequisition natural;
	gint           dw, dh;
	gint           max_w, max_h;

	/* --- monitor work area ----------------------------------------- */
	display = gtk_widget_get_display(widget);
	monitor = gdk_display_get_primary_monitor(display);
	if (!monitor)
		monitor = gdk_display_get_monitor(display, 0);
	gdk_monitor_get_workarea(monitor, &workarea);
	max_w = (gint)(workarea.width  * 0.95);
	max_h = (gint)(workarea.height * 0.95);

	/* --- natural content size --------------------------------------- */
	gtk_widget_get_preferred_size(widget, NULL, &natural);

	/* --- current default size (-1 = unset) ------------------------- */
	gtk_window_get_default_size(GTK_WINDOW(widget), &dw, &dh);
	if (dw <= 0) dw = natural.width;
	if (dh <= 0) dh = natural.height;

	/* --- clamp to work area ---------------------------------------- */
	dw = MIN(dw, max_w);
	dh = MIN(dh, max_h);

	gtk_window_set_default_size(GTK_WINDOW(widget), dw, dh);
}

/* Notes: */

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_window_clear(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Well, it can't be null because something changes it to
	 * "Unnamed Window", and since the default is PACKAGE when
	 * no title has been declared I'll clear it to that */

	gtk_window_set_title(GTK_WINDOW(var->Widget), PACKAGE);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/
GtkWidget *widget_window_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	gchar            *value;
	GError           *error = NULL;
	GList            *accel_group = NULL;
	GList            *element;
	gint              border_width;
	GtkWidget        *widget;
	stackelement      s;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Create the window widget */
	widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);  

	/* Set a default window title */
	attributeset_set_if_unset(Attr, ATTR_LABEL, PACKAGE);
	gtk_window_set_title(GTK_WINDOW(widget), 
		attributeset_get_first(&element, Attr, ATTR_LABEL));

	/* Set a default title bar theme icon */
	gtk_window_set_icon_name(GTK_WINDOW(widget), PACKAGE);

	/* If requested set a title bar image by filename */
	if (attr) {
		if ((value = get_tag_attribute(attr, "image-name")))
			gtk_window_set_icon_from_file(GTK_WINDOW(widget),
				find_pixmap(value), &error);
	}

	/* Set a default border width */
	border_width = 5;
	if (attr && (value = get_tag_attribute(attr, "margin")))	/* Deprecated */
		border_width = atoi(value);
	gtk_container_set_border_width(GTK_CONTAINER(widget), border_width);

	/* Tag attributes: resizable, default-width/height, width-request/height-request.
	 *
	 * default-width / default-height  → gtk_window_set_default_size()
	 *   The window opens at this size but GTK expands it automatically if
	 *   the content requires more space.  Use -1 to let GTK choose a dimension.
	 *   This is the recommended way to set an initial size.
	 *
	 * width-request / height-request  → gtk_widget_set_size_request()
	 *   Hard minimum: the window can never be smaller than these values.
	 *   Only set when you really need to enforce a minimum.
	 *
	 * resizable  (true/false, default true)
	 *   Controls whether the user can resize the window.
	 */
	if (attr) {
		gint dw = -1, dh = -1;
		if ((value = get_tag_attribute(attr, "default-width")))
			dw = atoi(value);
		if ((value = get_tag_attribute(attr, "default-height")))
			dh = atoi(value);
		if (dw != -1 || dh != -1)
			gtk_window_set_default_size(GTK_WINDOW(widget), dw, dh);

		if ((value = get_tag_attribute(attr, "width-request"))) {
			gint wr = atoi(value);
			gint hr = -1;
			if ((value = get_tag_attribute(attr, "height-request")))
				hr = atoi(value);
			gtk_widget_set_size_request(widget, wr, hr);
		} else if ((value = get_tag_attribute(attr, "height-request"))) {
			gtk_widget_set_size_request(widget, -1, atoi(value));
		}

		if ((value = get_tag_attribute(attr, "resizable")))
			gtk_window_set_resizable(GTK_WINDOW(widget),
				(g_ascii_strcasecmp(value, "false") != 0 &&
				 g_ascii_strcasecmp(value, "no")    != 0 &&
				 g_ascii_strcasecmp(value, "0")     != 0));
	}

	/* If we have geometry given in the command line, we set that */
	if (have_geometry_dxdy)
		gtk_widget_set_size_request(widget, geometry_dx, geometry_dy);
	if (have_geometry_xy)
		gtk_window_move(GTK_WINDOW(widget), geometry_x, geometry_y);
	if (option_centering)
		gtk_window_set_position(GTK_WINDOW(widget),
			GTK_WIN_POS_CENTER_ALWAYS);

	/* Auto-size: clamp to screen work area on realize (fires before show) */
	g_signal_connect(widget, "realize",
		G_CALLBACK(widget_window_on_realize), NULL);

	/* Pop the widgets that the window will contain and add them */
	s = pop();
	gtk_container_add(GTK_CONTAINER(widget), s.widgets[0]);

	/* Thunor: Each menu created will have an accelerator group
	 * for its menuitems which will require adding to the window */
	if (accel_groups) {
		accel_group = g_list_first(accel_groups);
		while (accel_group) {
			gtk_window_add_accel_group(GTK_WINDOW(widget),
				GTK_ACCEL_GROUP(accel_group->data));
#ifdef DEBUG
			fprintf(stderr, "%s: Adding accel_group=%p to window\n",
				__func__, accel_group->data);
#endif
			accel_group = accel_group->next;
		}
		g_list_free(accel_groups);
		accel_groups = NULL;
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_window_envvar_all_construct(variable *var)
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

gchar *widget_window_envvar_construct(GtkWidget *widget)
{
	gchar            *string;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Thunor: Variables are exported before a window is launched which
	 * can be a problem if launching from a launched window because
	 * launched windows are required to contain a variable that matches
	 * the name of the program's envvar and this results in the program
	 * being overwritten with the window title ;) It took me a day to
	 * find this issue because the error was being reported by the parser
	 * but the problem was located in action_launchwindow. So window
	 * variables can't be exported, they must be reserved for scripts.
	 * 
	 * string = g_strdup(gtk_window_get_title(GTK_WINDOW(widget))); */
	string = NULL;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_window_fileselect(
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
void widget_window_refresh(variable *var)
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
			widget_window_input_by_command(var, act + 8);
		/* input file stock = "File:", input file = "File:/path/to/file" */
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised) {
				/* Check for file-monitor and create if requested */
				widget_file_monitor_try_create(var, act + 5);
			}
			widget_window_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_window_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
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
		g_signal_connect(G_OBJECT(var->Widget), "delete-event",
			G_CALLBACK(window_delete_event_handler), NULL);

	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_window_removeselected(variable *var)
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

void widget_window_save(variable *var)
{
	FILE             *outfile;
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
			fprintf(outfile, "%s", gtk_window_get_title(GTK_WINDOW(var->Widget)));
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

static void widget_window_input_by_command(variable *var, char *command)
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

			gtk_window_set_title(GTK_WINDOW(var->Widget), line);

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

static void widget_window_input_by_file(variable *var, char *filename)
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

			gtk_window_set_title(GTK_WINDOW(var->Widget), line);

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

static void widget_window_input_by_items(variable *var)
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
