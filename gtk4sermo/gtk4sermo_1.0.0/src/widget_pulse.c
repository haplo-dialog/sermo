/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_pulse.c:
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
 * GtkProgressBar in pulse mode — indeterminate activity indicator.
 *
 * XML syntax:
 *   <pulse text="Traitement en cours..."></pulse>
 *
 *   <pulse>
 *     <variable>MY_PULSE</variable>
 *   </pulse>
 *
 * Tag attributes:
 *   text="string"       — optional text displayed on the bar
 *   show-text="true"    — show text (default: true when text= is set)
 *
 * Actions:
 *   <action>pulse:MY_PULSE</action>   — advance the pulse one step
 *   <action>clear:MY_PULSE</action>   — reset to 0
 *   <action>enable:MY_PULSE</action>
 *   <action>disable:MY_PULSE</action>
 *
 * <input> value:
 *   "pulse"  → gtk_progress_bar_pulse()
 *   "reset"  → gtk_progress_bar_set_fraction(0.0)
 *   any text → set label text on the bar
 *
 * Exported variable:
 *   MY_PULSE = "pulse" (constant — the bar has no numeric value in pulse mode)
 *
 * NOTE: to auto-animate, use a <timer> widget that fires <action>pulse:MY_PULSE</action>
 * periodically. The pulse widget itself does not auto-animate.
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

static void widget_pulse_input_by_command(variable *var, char *command);
static void widget_pulse_input_by_file(variable *var, char *filename);
static void widget_pulse_input_by_items(variable *var);

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_pulse_clear(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(var->Widget), 0.0);
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_pulse_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget   *widget;
	gchar       *text     = NULL;
	gchar       *showtext = NULL;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget = gtk_progress_bar_new();

	if (attr) {
		text     = get_tag_attribute(attr, "text");
		showtext = get_tag_attribute(attr, "show-text");
	}

	if (text) {
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(widget), text);
		gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(widget), TRUE);
	}
	if (showtext) {
		gboolean st = (g_ascii_strcasecmp(showtext, "true") == 0 ||
		               g_ascii_strcasecmp(showtext, "yes")  == 0 ||
		               strcmp(showtext, "1") == 0);
		gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(widget), st);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_pulse_envvar_all_construct(variable *var)
{
	return g_strdup("");
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_pulse_envvar_construct(GtkWidget *widget)
{
	return g_strdup("pulse");
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_pulse_fileselect(
	variable *var, const char *name, const char *value)
{
	g_warning("%s(): Fileselect not implemented for this widget.", __func__);
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/

void widget_pulse_refresh(variable *var)
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
			widget_pulse_input_by_command(var, act + 8);
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_pulse_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_pulse_input_by_items(var);

	if (!initialised) {
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
			gchar *def = attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT);
			if (def && *def) {
				gtk_progress_bar_set_text(GTK_PROGRESS_BAR(var->Widget), def);
				gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(var->Widget), TRUE);
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

void widget_pulse_removeselected(variable *var)
{
	g_warning("%s(): Removeselected not implemented for this widget.", __func__);
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_pulse_save(variable *var)
{
	g_warning("%s(): Save not implemented for this widget.", __func__);
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_pulse_input_by_command(variable *var, char *command)
{
	FILE  *infile;
	gchar  line[256];
	gint   count;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = widget_opencommand(command))) {
		if (fgets(line, sizeof(line), infile)) {
			line[sizeof(line) - 1] = 0;
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			if (g_ascii_strcasecmp(line, "pulse") == 0)
				gtk_progress_bar_pulse(GTK_PROGRESS_BAR(var->Widget));
			else if (g_ascii_strcasecmp(line, "reset") == 0 ||
			         g_ascii_strcasecmp(line, "clear") == 0)
				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(var->Widget), 0.0);
			else {
				gtk_progress_bar_set_text(GTK_PROGRESS_BAR(var->Widget), line);
				gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(var->Widget), TRUE);
			}
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

static void widget_pulse_input_by_file(variable *var, char *filename)
{
	FILE  *infile;
	gchar  line[256];
	gint   count;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = fopen(filename, "r"))) {
		if (fgets(line, sizeof(line), infile)) {
			line[sizeof(line) - 1] = 0;
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			if (g_ascii_strcasecmp(line, "pulse") == 0)
				gtk_progress_bar_pulse(GTK_PROGRESS_BAR(var->Widget));
			else if (g_ascii_strcasecmp(line, "reset") == 0 ||
			         g_ascii_strcasecmp(line, "clear") == 0)
				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(var->Widget), 0.0);
			else {
				gtk_progress_bar_set_text(GTK_PROGRESS_BAR(var->Widget), line);
				gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(var->Widget), TRUE);
			}
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

static void widget_pulse_input_by_items(variable *var)
{
	g_warning("%s(): <item> not implemented for this widget.", __func__);
}
