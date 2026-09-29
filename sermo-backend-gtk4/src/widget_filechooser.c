/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_filechooser.c:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
 * Copyright (C) 2011-2012  Thunor <thunorsif@hotmail.com>
 * Copyright (C) 2026  haplo-dialog <devel@haplo-dialog.fr> (GTK4 port)
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
 * GtkFileChooserButton — an inline button that opens a file/folder chooser.
 *
 * XML usage:
 *   <filechooser>
 *     <variable>MY_FILE</variable>
 *     <default>/home/user/documents</default>
 *     <action>echo "Selected: $MY_FILE"</action>
 *   </filechooser>
 *
 * Attributes:
 *   <label>   — button label / title for the dialog (default: "Select File")
 *   <default> — pre-selected path
 *
 * Tag attributes (tag_attr):
 *   action="select-folder"  — switch to folder selection mode
 *
 * Environment variable: absolute path of the selected file/folder,
 *   or empty string if none selected.
 * Signal: file-set — fired when the user confirms a selection.
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
#include "tag_attributes.h"
#include "sermo_input.h"

/* Defines */
//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

/* Local function prototypes, located at file bottom */
static void widget_filechooser_input_by_command(variable *var, char *command);
static void widget_filechooser_input_by_file(variable *var, char *filename);
static void widget_filechooser_input_by_items(variable *var);

/* Notes:
 * GtkFileChooserButton wraps GtkFileChooserDialog in a compact button.
 * The "file-set" signal fires when the user confirms their selection.
 * Use gtk_file_chooser_get_file() (GTK 2.14+, forward-compatible with GTK4)
 * rather than the deprecated gtk_file_chooser_get_filename().
 * GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER enables folder-only selection.
 */

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_filechooser_clear(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* GTK4 : GtkFileChooserButton n'existe plus, le shim est un GtkButton —
	 * le chemin selectionne vit en data "fc_path" (les gtk_file_chooser_*
	 * sur ce bouton ne faisaient qu'emettre des CRITICAL). */
	g_object_set_data_full(G_OBJECT(var->Widget), "fc_path", NULL, g_free);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/* ── Le VRAI sélecteur (GTK4) ─────────────────────────────────────────
 * GtkFileChooserButton n'existe plus : le widget est un bouton, et le
 * clic ouvre un GtkFileDialog (API asynchrone, GTK 4.10+). Le chemin
 * choisi vit en data "fc_path" et devient le label du bouton. */

static void filechooser_open_done(GObject *source, GAsyncResult *res,
	gpointer data)
{
	GtkButton *btn = GTK_BUTTON(data);
	GFile *f = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), res, NULL);
	if (f) {
		gchar *path = g_file_get_path(f);
		g_object_unref(f);
		if (path) {
			gtk_button_set_label(btn, path);
			g_object_set_data_full(G_OBJECT(btn), "fc_path", path, g_free);
		}
	}
}

static void filechooser_folder_done(GObject *source, GAsyncResult *res,
	gpointer data)
{
	GtkButton *btn = GTK_BUTTON(data);
	GFile *f = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source),
		res, NULL);
	if (f) {
		gchar *path = g_file_get_path(f);
		g_object_unref(f);
		if (path) {
			gtk_button_set_label(btn, path);
			g_object_set_data_full(G_OBJECT(btn), "fc_path", path, g_free);
		}
	}
}

static void filechooser_clicked_open(GtkWidget *btn, gpointer user_data)
{
	GtkFileDialog *dlg = gtk_file_dialog_new();
	GtkRoot *root = gtk_widget_get_root(btn);
	GtkWindow *parent = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : NULL;
	(void)user_data;
	if (g_object_get_data(G_OBJECT(btn), "fc_folder"))
		gtk_file_dialog_select_folder(dlg, parent, NULL,
			filechooser_folder_done, btn);
	else
		gtk_file_dialog_open(dlg, parent, NULL,
			filechooser_open_done, btn);
	g_object_unref(dlg);
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_filechooser_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GList                    *element;
	GtkWidget                *widget;
	GtkFileChooserAction      action = GTK_FILE_CHOOSER_ACTION_OPEN;
	const gchar              *label;
	const gchar              *action_str;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Check for folder-select mode via tag attribute */
	if (attr) {
		action_str = get_tag_attribute(attr, "action");
		if (action_str && strcmp(action_str, "select-folder") == 0)
			action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
	}

	attributeset_set_if_unset(Attr, ATTR_LABEL, "Select File");
	label = attributeset_get_first(&element, Attr, ATTR_LABEL);

	widget = gtk_file_chooser_button_new(label, action);

	/* Le clic ouvre le vrai sélecteur (GtkFileDialog) */
	if (action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
		g_object_set_data(G_OBJECT(widget), "fc_folder", GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(widget), "clicked",
		G_CALLBACK(filechooser_clicked_open), NULL);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_filechooser_envvar_all_construct(variable *var)
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

gchar *widget_filechooser_envvar_construct(GtkWidget *widget)
{
	GFile            *file;
	gchar            *path;
	gchar            *string = NULL;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* GTK4 : le chemin vit en data "fc_path" (shim GtkButton) */
	(void)file; (void)path;
	{
		const gchar *fp2 = g_object_get_data(G_OBJECT(widget), "fc_path");
		string = g_strdup(fp2 ? fp2 : "");
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_filechooser_fileselect(
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

void widget_filechooser_refresh(variable *var)
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
			widget_filechooser_input_by_command(var, act + 8);
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_filechooser_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_filechooser_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
			value = attributeset_get_first(&element, var->Attributes,
				ATTR_DEFAULT);
			if (value && *value) {
				/* GTK4 : chemin en data + label du bouton */
				g_object_set_data_full(G_OBJECT(var->Widget),
					"fc_path", g_strdup(value), g_free);
				gtk_button_set_label(GTK_BUTTON(var->Widget), value);
			}
		}
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
		/* GTK4 : le shim est un GtkButton — "file-set" n'existe pas
		 * (CRITICAL au demarrage). Le clic est la seule prise restante ;
		 * le VRAI selecteur (GtkFileDialog) reste une dette consignee. */
		g_signal_connect(G_OBJECT(var->Widget), "clicked",
			G_CALLBACK(on_any_widget_changed_event), (gpointer)var->Attributes);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_filechooser_removeselected(variable *var)
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

void widget_filechooser_save(variable *var)
{
	FILE             *outfile;
	GList            *element;
	GFile            *file;
	gchar            *act;
	gchar            *filename = NULL;
	gchar            *path;

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
			{
				const gchar *fp2 = g_object_get_data(
					G_OBJECT(var->Widget), "fc_path");
				if (fp2) fprintf(outfile, "%s", fp2);
			}
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

static void widget_filechooser_input_by_command(variable *var, char *command)
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
			if (*line)
				gtk_file_chooser_set_filename(
					GTK_FILE_CHOOSER(var->Widget), line);
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

static void widget_filechooser_input_by_file(variable *var, char *filename)
{
	FILE             *infile;
	gchar             line[512];
	gint              count;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = sermo_fopen_input(filename))) {
		if (fgets(line, 512, infile)) {
			line[511] = 0;
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			if (*line)
				gtk_file_chooser_set_filename(
					GTK_FILE_CHOOSER(var->Widget), line);
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

static void widget_filechooser_input_by_items(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning("%s(): <item> not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}
