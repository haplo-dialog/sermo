/*
 * widget_list.c: 
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
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "tag_attributes.h"

/* GtkListBox::row-selected passe UN PARAMÈTRE DE PLUS que les signaux sans
 * argument : (box, row, data). Le brancher directement sur
 * on_any_widget_selection_changed_event(widget, Attr) faisait recevoir la ligne
 * sélectionnée à la place du jeu d'attributs, qui était ensuite déréférencé —
 * segfault à la première sélection, sur les deux ports. */
static void hp_list_row_selected(GtkListBox *box, GtkListBoxRow *row,
	gpointer data)
{
	(void)row;
	on_any_widget_selection_changed_event(GTK_WIDGET(box),
		(AttributeSet *)data);
}

/* Defines */
//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

/* Local function prototypes, located at file bottom */
static void widget_list_input_by_command(variable *var, char *filename,
	gint command_or_file);
static void widget_list_input_by_file(variable *var, char *filename);
static void widget_list_input_by_items(variable *var);

/* Notes:
 * Using this on Puppy lupu 520:
 * 
 * find /sbin /usr -type f -exec grep -qI "gtk4sermo" {} \; -print > installed_gtkdialog_apps
 * for f in `cat ./installed_gtkdialog_apps`; do grep -Hn "<list" $f; done
 * 
 * Resulted in this:
 * 
 * /usr/local/apps/pnethood/pnethood:159:    <list><height>300</height>
 * 
 * And it's only using <list> to present data (no actions) so I'm going
 * to tweak this widget (0.8.1 2012-09-09) to make it usable although it
 * appears to be well past its sell-by-date since GTK_SELECTION_MULTIPLE
 * won't work.
 */

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_list_clear(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* GTK3: gtk_list_clear_items removed — iterate and destroy all GtkListBoxRows */
	{
		GList *children = gtk_container_get_children(GTK_CONTAINER(var->Widget));
		GList *l;
		for (l = children; l != NULL; l = l->next)
			gtk_widget_destroy(GTK_WIDGET(l->data));
		g_list_free(children);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/
GtkWidget *widget_list_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget        *widget;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* GTK3: GtkList removed — use GtkListBox as a drop-in replacement */
	widget = gtk_list_box_new();

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_list_envvar_all_construct(variable *var)
{
	gchar            *string = NULL;

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

	return string;
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_list_envvar_construct(GtkWidget *widget)
{
	GList            *item;
	gchar            *string = NULL;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Thunor: 2012-09-09 GTK_SELECTION_MULTIPLE does not work. According
	 * to http://developer.gnome.org/gtk-faq/stable/c707.html, selection
	 * should return a pointer to the first selected item but it returns
	 * nothing but NULL in that mode so I can't implement it */

	/* GTK3: GTK_LIST()->selection → gtk_list_box_get_selected_row() */
	{
		GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(widget));
		if (row != NULL) {
			string = g_object_get_data(G_OBJECT(row), "user_data");
		} else {
			string = g_strdup("");
		}
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_list_fileselect(
	variable *var, const char *name, const char *value)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	fprintf(stderr, "%s(): Fileselect not implemented for this widget.\n", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/
void widget_list_refresh(variable *var)
{
	GList            *element;
	gchar            *act;
	gchar            *value;
	gint              initialised = FALSE;
	gint              selected_row;

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
			widget_list_input_by_command(var, act + 8, TRUE);
		/* input file stock = "File:", input file = "File:/path/to/file" */
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised) {
				/* Check for file-monitor and create if requested */
				widget_file_monitor_try_create(var, act + 5);
			}
			widget_list_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_list_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
		if (attributeset_is_avail(var->Attributes, ATTR_LABEL))
			fprintf(stderr, "%s(): <label> not implemented for this widget.\n",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT))
			fprintf(stderr, "%s(): <default> not implemented for this widget.\n",
				__func__);
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||	/* Deprecated */
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);

		/* Connect signals — GTK3: GtkListBox uses "row-selected" not "selection-changed" */
		g_signal_connect(G_OBJECT(var->Widget), "row-selected",
			G_CALLBACK(hp_list_row_selected),
			(gpointer)var->Attributes);

	}

	/* Thunor: This was executed after filling by command and item
	 * so I've put it here and made it optional */
	if (var->widget_tag_attr) {
		/* Get selected-row (custom) */
		if ((value = get_tag_attribute(var->widget_tag_attr, "selected-row"))) {
			selected_row = atoi(value);
			if (selected_row >= 0)
				/* GTK3: gtk_list_select_item → gtk_list_box_select_row */
			gtk_list_box_select_row(GTK_LIST_BOX(var->Widget),
				gtk_list_box_get_row_at_index(GTK_LIST_BOX(var->Widget), selected_row));
		}
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_list_removeselected(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

#ifdef DEBUG_CONTENT
	/* GTK3: GtkListBox selection debug */
	{
		GList *sel = gtk_list_box_get_selected_rows(GTK_LIST_BOX(var->Widget));
		GList *l   = g_list_first(sel);
		fprintf(stderr, "%s(): selected rows=%p\n", __func__, sel);
		while (l) {
			if (GTK_IS_LIST_BOX_ROW(l->data))
				fprintf(stderr, "%s():   row data=%s\n", __func__,
					(gchar*)g_object_get_data(G_OBJECT(l->data), "user_data"));
			l = g_list_next(l);
		}
		g_list_free(sel);
	}
#endif

	/* GTK3: GTK_LIST()->selection → gtk_list_box_get_selected_row() */
	{
		GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(var->Widget));
		if (row != NULL)
			gtk_widget_destroy(GTK_WIDGET(row));
	}
	/* Note: auto-selection of first item after removal is not done here
	 * (was also commented out in the original code). */

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_list_save(variable *var)
{
	FILE             *outfile;
	GList            *child;
	GList            *element;
	GList            *listchildren = NULL;
	gchar            *act;
	gchar            *filename = NULL;
	gchar            *string = NULL;
	gint              count = 0;

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

			/* GTK3: GtkList → GtkListBox. Rows are GtkListBoxRow with a GtkLabel
			 * child. The item text is stored as "user_data" on each row. */
			listchildren = gtk_container_get_children(GTK_CONTAINER(var->Widget));
			child = g_list_first(listchildren);
			while (child) {
				if (GTK_IS_LIST_BOX_ROW(child->data)) {
					string = g_object_get_data(G_OBJECT(child->data), "user_data");
#ifdef DEBUG_CONTENT
					fprintf(stderr, "%s(): child->data='%s'\n", __func__, string);
#endif
					if (count == 0) {
						fprintf(outfile, "%s", string);
					} else {
						fprintf(outfile, "\n%s", string);
					}
					count++;
				}
				child = child->next;
			}
			if (listchildren) {
				g_list_free(listchildren);
				listchildren = NULL;
			}

			/* Close the file */
			fclose(outfile);
		} else {
			fprintf(stderr, "%s(): Couldn't open '%s' for writing.\n",
				__func__, filename);
		}
	} else {
		fprintf(stderr, "%s(): No <output file> directive found.\n", __func__);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_list_input_by_command(variable *var, char *filename,
	gint command_or_file)
{
	FILE             *infile;
	GtkWidget        *item;
	gchar             line[512];
	gchar            *saved;
	gint              count;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (command_or_file) {
		infile = widget_opencommand(filename);
	} else {
		infile = fopen(filename, "r");
	}

	/* Opening pipe for reading... */
	if (infile) {
		/* Read the file one line at a time (trailing [CR]LFs are read too) */
		while (fgets(line, 512, infile) != NULL) {
			/* Enforce end of string in case of max chars read */
			line[512 - 1] = 0;
			/* Remove the trailing [CR]LFs */
			for (count = strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;

			/* GTK3: gtk_list_item_new_with_label → GtkListBoxRow + GtkLabel */
			saved = g_strdup(line);
			item = gtk_list_box_row_new();
			{
				GtkWidget *label = gtk_label_new(line);
				gtk_label_set_xalign(GTK_LABEL(label), 0.0); /* left-align text */
				gtk_container_add(GTK_CONTAINER(item), label);
			}
			g_object_set_data(G_OBJECT(item), "user_data", (gpointer)saved);
			gtk_container_add(GTK_CONTAINER(var->Widget), item);
			gtk_widget_show_all(item);

		}
		/* safe_popen() uses fdopen() → fclose(), not pclose() */
		fclose(infile);
	} else {
		fprintf(stderr, "%s(): Couldn't open '%s' for reading.\n", __func__,
			filename);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by File                                                       *
 ***********************************************************************/

static void widget_list_input_by_file(variable *var, char *filename)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget_list_input_by_command(var, filename, FALSE);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Items                                                      *
 ***********************************************************************/

static void widget_list_input_by_items(variable *var)
{
	GList            *element;
	GtkWidget        *item;
	gchar            *text;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Thunor: This is all original code moved across when refactoring */
	g_assert(var->Attributes != NULL && var->Widget != NULL);

	text = attributeset_get_first(&element, var->Attributes, ATTR_ITEM);
	if (text == NULL)
		return;

	while (text != NULL) {
		/* GTK3: gtk_list_item_new_with_label → GtkListBoxRow + GtkLabel */
		item = gtk_list_box_row_new();
		{
			GtkWidget *label = gtk_label_new(text);
			gtk_label_set_xalign(GTK_LABEL(label), 0.0); /* left-align text */
			gtk_container_add(GTK_CONTAINER(item), label);
		}
		g_object_set_data(G_OBJECT(item), "user_data", (gpointer)text);
		gtk_container_add(GTK_CONTAINER(var->Widget), item);
		gtk_widget_show_all(item);
		text = attributeset_get_next(&element, var->Attributes, ATTR_ITEM);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}
