/*
 * widget_infobar.c:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
 * Copyright (C) 2011-2012  Thunor <thunorsif@hotmail.com>
 * Copyright (C) 2026  Haplo-Linux <devel@haplo-dialog.fr> (GTK4 port)
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
 * GtkInfoBar — a horizontal bar for inline status/information messages.
 *
 * XML usage:
 *   <infobar>
 *     <variable>STATUS_BAR</variable>
 *     <label>Operation completed successfully.</label>
 *     <default>info</default>
 *     <action>echo "Dismissed"</action>
 *   </infobar>
 *
 * The <default> attribute sets the message type:
 *   "info"      → GTK_MESSAGE_INFO     (blue, default)
 *   "warning"   → GTK_MESSAGE_WARNING  (yellow)
 *   "error"     → GTK_MESSAGE_ERROR    (red)
 *   "question"  → GTK_MESSAGE_QUESTION
 *   "other"     → GTK_MESSAGE_OTHER    (no icon)
 *
 * The <label> attribute sets the displayed message text.
 * Environment variable: the current message text.
 * Signal: response — fired when user clicks a button (e.g. dismiss).
 *
 * Dynamic update: send a new message via <input> or shell command.
 * To change the type at runtime, use a refresh with updated <default>.
 */

/* Includes */
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

/* Defines */
//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

/* Local function prototypes, located at file bottom */
static void widget_infobar_input_by_command(variable *var, char *command);
static void widget_infobar_input_by_file(variable *var, char *filename);
static void widget_infobar_input_by_items(variable *var);

/* Notes:
 * GtkInfoBar contains a GtkLabel in its content area.
 * We store a pointer to the label as object data "_label" on the infobar.
 * Message type is set at create time from <default>; changing it at runtime
 * requires recreating the infobar (limitation of GtkInfoBar API in GTK3).
 */

/* Helper: parse message type string */
static GtkMessageType infobar_message_type_from_string(const gchar *s)
{
	if (!s) return GTK_MESSAGE_INFO;
	if (strcasecmp(s, "warning")  == 0) return GTK_MESSAGE_WARNING;
	if (strcasecmp(s, "error")    == 0) return GTK_MESSAGE_ERROR;
	if (strcasecmp(s, "question") == 0) return GTK_MESSAGE_QUESTION;
	if (strcasecmp(s, "other")    == 0) return GTK_MESSAGE_OTHER;
	return GTK_MESSAGE_INFO; /* default */
}

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_infobar_clear(variable *var)
{
	GtkWidget        *label;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	label = GTK_WIDGET(g_object_get_data(G_OBJECT(var->Widget), "_label"));
	if (label)
		gtk_label_set_text(GTK_LABEL(label), "");

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_infobar_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GList            *element;
	GtkWidget        *widget;
	GtkWidget        *content_area;
	GtkWidget        *label;
	GtkMessageType    msg_type = GTK_MESSAGE_INFO;
	const gchar      *msg_text = "";
	const gchar      *type_str;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Message type from <default> */
	if (attributeset_is_avail(Attr, ATTR_DEFAULT)) {
		type_str = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
		msg_type = infobar_message_type_from_string(type_str);
	}

	/* Message text from <label> */
	if (attributeset_is_avail(Attr, ATTR_LABEL))
		msg_text = attributeset_get_first(&element, Attr, ATTR_LABEL);

	widget = gtk_info_bar_new();
	gtk_info_bar_set_message_type(GTK_INFO_BAR(widget), msg_type);
	gtk_info_bar_set_show_close_button(GTK_INFO_BAR(widget), TRUE);

	/* Add a label to the content area */
	content_area = gtk_info_bar_get_content_area(GTK_INFO_BAR(widget));
	label = gtk_label_new(msg_text);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
	gtk_container_add(GTK_CONTAINER(content_area), label);
	gtk_widget_show(label);

	/* Store reference to the label for later updates */
	g_object_set_data(G_OBJECT(widget), "_label", label);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_infobar_envvar_all_construct(variable *var)
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

gchar *widget_infobar_envvar_construct(GtkWidget *widget)
{
	GtkWidget        *label;
	gchar            *string = NULL;
	const gchar      *text;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "_label"));
	if (label) {
		text = gtk_label_get_text(GTK_LABEL(label));
		string = g_strdup(text ? text : "");
	} else {
		string = g_strdup("");
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_infobar_fileselect(
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

void widget_infobar_refresh(variable *var)
{
	GList            *element;
	gchar            *act;
	gint              initialised = FALSE;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Get initialised state of widget */
	if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
		initialised = GPOINTER_TO_INT(
			g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

	/* The <input> tag — updates the displayed message text */
	act = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
	while (act) {
		if (input_is_shell_command(act))
			widget_infobar_input_by_command(var, act + 8);
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_infobar_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_infobar_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		if (attributeset_is_avail(var->Attributes, ATTR_HEIGHT))
			g_warning("%s(): <height> not implemented for this widget.",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_WIDTH))
			g_warning("%s(): <width> not implemented for this widget.",
				__func__);
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);

		/* Connect signals */
		g_signal_connect(G_OBJECT(var->Widget), "response",
			G_CALLBACK(on_any_widget_changed_event), (gpointer)var->Attributes);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_infobar_removeselected(variable *var)
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

void widget_infobar_save(variable *var)
{
	FILE             *outfile;
	GList            *element;
	GtkWidget        *label;
	gchar            *act;
	gchar            *filename = NULL;
	const gchar      *text;

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
			label = GTK_WIDGET(
				g_object_get_data(G_OBJECT(var->Widget), "_label"));
			text = label ? gtk_label_get_text(GTK_LABEL(label)) : "";
			fprintf(outfile, "%s", text ? text : "");
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

static void widget_infobar_input_by_command(variable *var, char *command)
{
	FILE             *infile;
	GtkWidget        *label;
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
			label = GTK_WIDGET(
				g_object_get_data(G_OBJECT(var->Widget), "_label"));
			if (label)
				gtk_label_set_text(GTK_LABEL(label), line);
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

static void widget_infobar_input_by_file(variable *var, char *filename)
{
	FILE             *infile;
	GtkWidget        *label;
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
			label = GTK_WIDGET(
				g_object_get_data(G_OBJECT(var->Widget), "_label"));
			if (label)
				gtk_label_set_text(GTK_LABEL(label), line);
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

static void widget_infobar_input_by_items(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning("%s(): <item> not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}
