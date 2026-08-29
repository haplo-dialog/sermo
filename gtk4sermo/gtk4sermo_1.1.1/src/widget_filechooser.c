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

	hp_filechooser_set_path(var->Widget, NULL);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/


/* ─────────────────────────────────────────────────────────────────────────
 * Implémentation réelle du <filechooser> en GTK 4.
 *
 * GtkFileChooserButton n'existe plus. On rend un GtkButton qui ouvre un
 * GtkFileDialog au clic. GtkFileDialog est ASYNCHRONE : on ne bloque pas la
 * boucle principale, le chemin choisi est mémorisé sur le bouton par le
 * rappel, et c'est LUI qui déclenche ensuite les <action> — exactement ce que
 * faisait le signal "file-set" du port GTK 3.
 * ───────────────────────────────────────────────────────────────────────── */

#define HP_FC_PATH   "gtk4sermo-fc-path"
#define HP_FC_ATTRS  "gtk4sermo-fc-attrs"
#define HP_FC_ACTION "gtk4sermo-fc-action"

void hp_filechooser_set_path(gpointer bouton, const char *chemin)
{
    GtkWidget *b = GTK_WIDGET(bouton);

    if (b == NULL) return;

    g_object_set_data_full(G_OBJECT(b), HP_FC_PATH,
                           (chemin && *chemin) ? g_strdup(chemin) : NULL, g_free);

    /* Le bouton affiche le nom du fichier, comme le faisait GtkFileChooserButton. */
    if (GTK_IS_BUTTON(b)) {
        if (chemin && *chemin) {
            gchar *base = g_path_get_basename(chemin);
            gtk_button_set_label(GTK_BUTTON(b), base);
            g_free(base);
        } else {
            const gchar *t = g_object_get_data(G_OBJECT(b), "gtk4sermo-fc-title");
            gtk_button_set_label(GTK_BUTTON(b), (t && *t) ? t : "Select File");
        }
    }
}

GFile *hp_filechooser_get_file(gpointer bouton)
{
    const gchar *chemin;

    if (bouton == NULL || !G_IS_OBJECT(bouton)) return NULL;
    chemin = g_object_get_data(G_OBJECT(bouton), HP_FC_PATH);
    return (chemin && *chemin) ? g_file_new_for_path(chemin) : NULL;
}

void hp_filechooser_connect(gpointer bouton, gpointer attrs)
{
    if (bouton && G_IS_OBJECT(bouton))
        g_object_set_data(G_OBJECT(bouton), HP_FC_ATTRS, attrs);
}

static void hp_fc_choisi(GObject *source, GAsyncResult *res, gpointer data)
{
    GtkWidget    *b   = GTK_WIDGET(data);
    GtkFileDialog *d  = GTK_FILE_DIALOG(source);
    AttributeSet *Attr;
    GFile        *f;
    GError       *err = NULL;
    gint          action;

    action = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), HP_FC_ACTION));
    f = (action == 1) ? gtk_file_dialog_select_folder_finish(d, res, &err)
                      : gtk_file_dialog_open_finish(d, res, &err);

    if (f == NULL) {
        /* Annulation : ce n'est pas une erreur, on ne touche a rien. */
        if (err) g_error_free(err);
        return;
    }

    {
        gchar *chemin = g_file_get_path(f);
        hp_filechooser_set_path(b, chemin);
        g_free(chemin);
    }
    g_object_unref(f);

    /* Equivalent du signal "file-set" du port GTK 3. */
    Attr = g_object_get_data(G_OBJECT(b), HP_FC_ATTRS);
    if (Attr) on_any_widget_changed_event(b, Attr);
}

static void hp_fc_clic(GtkButton *b, gpointer data)
{
    GtkFileDialog *d = gtk_file_dialog_new();
    GtkRoot       *r = gtk_widget_get_root(GTK_WIDGET(b));
    GtkWindow     *parent = GTK_IS_WINDOW(r) ? GTK_WINDOW(r) : NULL;
    const gchar   *titre = g_object_get_data(G_OBJECT(b), "gtk4sermo-fc-title");
    gint           action = GPOINTER_TO_INT(data);

    if (titre && *titre) gtk_file_dialog_set_title(d, titre);

    if (action == 1)
        gtk_file_dialog_select_folder(d, parent, NULL, hp_fc_choisi, b);
    else
        gtk_file_dialog_open(d, parent, NULL, hp_fc_choisi, b);

    g_object_unref(d);
}

GtkWidget *hp_filechooser_new(const char *titre, int action)
{
    /* action : 1 = dossier (GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER), sinon fichier */
    gint  mode = (action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) ? 1 : 0;
    GtkWidget *b = gtk_button_new_with_label((titre && *titre) ? titre : "Select File");

    g_object_set_data_full(G_OBJECT(b), "gtk4sermo-fc-title",
                           g_strdup(titre ? titre : ""), g_free);
    g_object_set_data(G_OBJECT(b), HP_FC_ACTION, GINT_TO_POINTER(mode));
    g_signal_connect(b, "clicked", G_CALLBACK(hp_fc_clic), GINT_TO_POINTER(mode));
    return b;
}


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

	/* gtk_file_chooser_get_file() is the GTK4-forward-compatible API */
	file = hp_filechooser_get_file(widget);
	if (file) {
		path = g_file_get_path(file);
		g_object_unref(file);
		if (path) {
			string = path; /* ownership transferred */
		} else {
			string = g_strdup("");
		}
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
			if (value && *value)
				hp_filechooser_set_path(var->Widget, value);
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
		hp_filechooser_connect(var->Widget, var->Attributes);
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
			file = hp_filechooser_get_file(var->Widget);
			if (file) {
				path = g_file_get_path(file);
				g_object_unref(file);
				if (path) {
					fprintf(outfile, "%s", path);
					g_free(path);
				}
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
				hp_filechooser_set_path(var->Widget, line);
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

	if ((infile = fopen(filename, "r"))) {
		if (fgets(line, 512, infile)) {
			line[511] = 0;
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;
			if (*line)
				hp_filechooser_set_path(var->Widget, line);
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
