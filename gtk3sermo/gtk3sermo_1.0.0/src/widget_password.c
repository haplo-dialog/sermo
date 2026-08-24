/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_password.c:
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
 * GtkEntry with visibility=FALSE — masked password field.
 *
 * XML syntax:
 *   <password>
 *     <variable>USER_PASS</variable>
 *   </password>
 *
 *   <password show-toggle="true" placeholder="Mot de passe...">
 *     <variable>USER_PASS</variable>
 *   </password>
 *
 * Tag attributes:
 *   show-toggle="true|false"   — show the eye icon to toggle visibility (default: true)
 *   placeholder="text"         — placeholder text (hint in the empty field)
 *   max-length="N"             — maximum number of characters
 *
 * Exported variable:
 *   USER_PASS = the text entered (plaintext — handle with care in scripts)
 *
 * NOTE: the value is exported as plaintext. Never log it or pass it as
 * a command-line argument; use a pipe or a tmpfs file instead.
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

static void widget_password_input_by_command(variable *var, char *command);
static void widget_password_input_by_file(variable *var, char *filename);
static void widget_password_input_by_items(variable *var);

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_password_clear(variable *var)
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

GtkWidget *widget_password_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget   *widget;
	gchar       *show_toggle = NULL;
	gchar       *placeholder = NULL;
	gchar       *maxlen      = NULL;
	gboolean     toggle      = TRUE;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(widget), FALSE);

	if (attr) {
		show_toggle = get_tag_attribute(attr, "show-toggle");
		placeholder = get_tag_attribute(attr, "placeholder");
		maxlen      = get_tag_attribute(attr, "max-length");
	}

	/* Eye icon to toggle visibility */
	if (show_toggle) {
		toggle = !(g_ascii_strcasecmp(show_toggle, "false") == 0 ||
		           g_ascii_strcasecmp(show_toggle, "no")    == 0 ||
		           strcmp(show_toggle, "0") == 0);
	}
	if (toggle) {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(widget),
		    GTK_ENTRY_ICON_SECONDARY, "view-reveal-symbolic");
		gtk_entry_set_icon_activatable(GTK_ENTRY(widget),
		    GTK_ENTRY_ICON_SECONDARY, TRUE);
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(widget),
		    GTK_ENTRY_ICON_SECONDARY, "Afficher / masquer le mot de passe");
	}

	if (placeholder)
		gtk_entry_set_placeholder_text(GTK_ENTRY(widget), placeholder);

	if (maxlen && atoi(maxlen) > 0)
		gtk_entry_set_max_length(GTK_ENTRY(widget), atoi(maxlen));

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_password_envvar_all_construct(variable *var)
{
	return g_strdup("");
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_password_envvar_construct(GtkWidget *widget)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif
	return g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_password_fileselect(
	variable *var, const char *name, const char *value)
{
	g_warning("%s(): Fileselect not implemented for this widget.", __func__);
}

/***********************************************************************
 * Toggle visibility callback                                          *
 ***********************************************************************/

static void on_password_icon_press(GtkEntry            *entry,
                                    GtkEntryIconPosition icon_pos,
                                    GdkEvent            *event,
                                    gpointer             user_data)
{
	if (icon_pos == GTK_ENTRY_ICON_SECONDARY) {
		gboolean visible = gtk_entry_get_visibility(entry);
		gtk_entry_set_visibility(entry, !visible);
		gtk_entry_set_icon_from_icon_name(entry,
		    GTK_ENTRY_ICON_SECONDARY,
		    visible ? "view-reveal-symbolic" : "view-conceal-symbolic");
	}
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/

void widget_password_refresh(variable *var)
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
			widget_password_input_by_command(var, act + 8);
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_password_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_password_input_by_items(var);

	if (!initialised) {
		if (attributeset_is_avail(var->Attributes, ATTR_LABEL))
			g_warning("%s(): <label> not implemented for this widget.", __func__);
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
			gchar *def = attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT);
			if (def) gtk_entry_set_text(GTK_ENTRY(var->Widget), def);
		}
		if (attributeset_is_avail(var->Attributes, ATTR_HEIGHT))
			g_warning("%s(): <height> not implemented for this widget.", __func__);
		if (attributeset_is_avail(var->Attributes, ATTR_WIDTH))
			gtk_widget_set_size_request(var->Widget,
			    atoi(attributeset_get_first(&element, var->Attributes, ATTR_WIDTH)), -1);
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);

		/* Connect icon-press for toggle */
		g_signal_connect(G_OBJECT(var->Widget), "icon-press",
		    G_CALLBACK(on_password_icon_press), NULL);
		/* Activate (Enter key) triggers actions */
		g_signal_connect(G_OBJECT(var->Widget), "activate",
		    G_CALLBACK(on_any_widget_activate_event),
		    (gpointer)var->Attributes);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_password_removeselected(variable *var)
{
	g_warning("%s(): Removeselected not implemented for this widget.", __func__);
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_password_save(variable *var)
{
	FILE  *outfile;
	GList *element;
	gchar *act;
	gchar *filename = NULL;

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
			fprintf(outfile, "%s", gtk_entry_get_text(GTK_ENTRY(var->Widget)));
			fclose(outfile);
		} else {
			g_warning("%s(): Couldn't open '%s' for writing.", __func__, filename);
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

static void widget_password_input_by_command(variable *var, char *command)
{
	FILE  *infile;
	gchar  line[512];
	gint   count;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = widget_opencommand(command))) {
		if (fgets(line, sizeof(line), infile)) {
			line[sizeof(line) - 1] = 0;
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

static void widget_password_input_by_file(variable *var, char *filename)
{
	FILE  *infile;
	gchar  line[512];
	gint   count;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = fopen(filename, "r"))) {
		if (fgets(line, sizeof(line), infile)) {
			line[sizeof(line) - 1] = 0;
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

static void widget_password_input_by_items(variable *var)
{
	g_warning("%s(): <item> not implemented for this widget.", __func__);
}
