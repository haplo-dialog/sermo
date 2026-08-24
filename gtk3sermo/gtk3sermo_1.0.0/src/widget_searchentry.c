/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_searchentry.c:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
 * Copyright (C) 2011-2012  Thunor <thunorsif@hotmail.com>
 * Copyright (C) 2026  Haplo-Linux <devel@haplo-dialog.fr> (GTK3 new widget)
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

/*
 * GtkSearchEntry — a GtkEntry variant styled for search with a clear icon.
 *
 * XML usage:
 *   <searchentry>
 *     <variable>SEARCH_TERM</variable>
 *     <default>type here...</default>
 *     <action>grep "$SEARCH_TERM" /some/file</action>
 *   </searchentry>
 *
 * Environment variable: current text content of the search field.
 * Signals:
 *   search-changed  — fires on each keystroke (with small delay)
 *   activate        — fires when user presses Enter
 *
 * GtkSearchEntry is a GtkEntry subclass, so all GtkEntry APIs apply.
 * The magnifier icon is added automatically; a clear button appears
 * when the field is non-empty.
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
static void widget_searchentry_input_by_command(variable *var, char *command);
static void widget_searchentry_input_by_file(variable *var, char *filename);
static void widget_searchentry_input_by_items(variable *var);

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_searchentry_clear(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	gtk_entry_set_text(GTK_ENTRY(var->Widget), "");

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_searchentry_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GList            *element;
	GtkWidget        *widget;
	const gchar      *placeholder;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget = gtk_search_entry_new();

	/* Use <label> as placeholder text if provided */
	if (attributeset_is_avail(Attr, ATTR_LABEL)) {
		placeholder = attributeset_get_first(&element, Attr, ATTR_LABEL);
		if (placeholder)
			gtk_entry_set_placeholder_text(GTK_ENTRY(widget), placeholder);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_searchentry_envvar_all_construct(variable *var)
{
	gchar            *string = NULL;  /* initialized: function not connected-up by default */

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* This function is a stub not connected-up by default; returns NULL. */

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s(): Hello.\n", __func__);
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_searchentry_envvar_construct(GtkWidget *widget)
{
	gchar            *string;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	string = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_searchentry_fileselect(
	variable *var, const char *name, const char *value)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning("%s(): Fileselect not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/

void widget_searchentry_refresh(variable *var)
{
	GList            *element;
	gchar            *act;
	gchar            *value;
	gint              initialised = FALSE;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Get initialised state of widget */
	if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
		initialised = GPOINTER_TO_INT(
			g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

	/* The <input> tag... */
	act = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
	while (act) {
		if (input_is_shell_command(act))
			widget_searchentry_input_by_command(var, act + 8);
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_searchentry_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_searchentry_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
			value = attributeset_get_first(&element, var->Attributes,
				ATTR_DEFAULT);
			gtk_entry_set_text(GTK_ENTRY(var->Widget), value ? value : "");
		}
		if (attributeset_is_avail(var->Attributes, ATTR_HEIGHT))
			g_warning("%s(): <height> not implemented for this widget.",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_WIDTH)) {
			value = attributeset_get_first(&element, var->Attributes,
				ATTR_WIDTH);
			gtk_entry_set_width_chars(GTK_ENTRY(var->Widget), atoi(value));
		}
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);

		/* Connect signals — search-changed fires after a short timeout */
		g_signal_connect(G_OBJECT(var->Widget), "search-changed",
			G_CALLBACK(on_any_widget_changed_event), (gpointer)var->Attributes);
		g_signal_connect(G_OBJECT(var->Widget), "activate",
			G_CALLBACK(on_any_widget_activate_event), (gpointer)var->Attributes);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_searchentry_removeselected(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning("%s(): Removeselected not implemented for this widget.",
		__func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_searchentry_save(variable *var)
{
	FILE             *outfile;
	GList            *element;
	gchar            *act;
	gchar            *filename = NULL;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	act = attributeset_get_first(&element, var->Attributes, ATTR_OUTPUT);
	while (act) {
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			filename = act + 5;
			break;
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_OUTPUT);
	}

	if (filename) {
		if ((outfile = fopen(filename, "w"))) {
			fprintf(outfile, "%s",
				gtk_entry_get_text(GTK_ENTRY(var->Widget)));
			fclose(outfile);
		} else {
			g_warning("%s(): Couldn't open '%s' for writing.",
				__func__, filename);
		}
	} else {
		g_warning("%s(): No <output file> directive found.", __func__);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_searchentry_input_by_command(variable *var, char *command)
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

	if ((infile = widget_opencommand(command))) {
		if (fgets(line, 512, infile)) {
			line[511] = 0;
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			gtk_entry_set_text(GTK_ENTRY(var->Widget), line);
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

static void widget_searchentry_input_by_file(variable *var, char *filename)
{
	FILE             *infile;
	gchar             line[512];
	gint              count;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = fopen(filename, "r"))) {
		if (fgets(line, 512, infile)) {
			line[511] = 0;
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			gtk_entry_set_text(GTK_ENTRY(var->Widget), line);
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

static void widget_searchentry_input_by_items(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning("%s(): <item> not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}
