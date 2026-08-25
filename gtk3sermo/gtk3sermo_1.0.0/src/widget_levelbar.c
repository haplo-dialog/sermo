/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_levelbar.c:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2026  haplo-dialog <devel@haplo-dialog.fr>
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
 * GtkLevelBar — affichage non interactif d'un niveau (jauge).
 *
 * Syntaxe XML :
 *   <levelbar>
 *     <variable>MY_LEVEL</variable>
 *     <input>echo 0.7</input>          <!-- valeur courante -->
 *   </levelbar>
 *
 *   <levelbar range-min="0" range-max="100" mode="discrete"></levelbar>
 *
 * Attributs de tag :
 *   range-min="x"   — valeur minimale (défaut : 0.0)
 *   range-max="x"   — valeur maximale (défaut : 1.0)
 *   mode="continuous|discrete"  — mode d'affichage (défaut : continuous)
 *
 * Variable exportée :
 *   MY_LEVEL=<valeur courante>
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

static void widget_levelbar_input_by_command(variable *var, char *command);
static void widget_levelbar_input_by_file(variable *var, char *filename);
static void widget_levelbar_input_by_items(variable *var);

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_levelbar_clear(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif
	gtk_level_bar_set_value(GTK_LEVEL_BAR(var->Widget), 0.0);
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_levelbar_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget  *widget;
	gchar      *value;
	gdouble     range_min = 0.0;
	gdouble     range_max = 1.0;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (attr) {
		if ((value = get_tag_attribute(attr, "range-min")))
			range_min = g_ascii_strtod(value, NULL);
		if ((value = get_tag_attribute(attr, "range-max")))
			range_max = g_ascii_strtod(value, NULL);
	}

	widget = gtk_level_bar_new_for_interval(range_min, range_max);

	if (attr) {
		if ((value = get_tag_attribute(attr, "mode"))) {
			if (g_ascii_strcasecmp(value, "discrete") == 0)
				gtk_level_bar_set_mode(GTK_LEVEL_BAR(widget),
					GTK_LEVEL_BAR_MODE_DISCRETE);
			else
				gtk_level_bar_set_mode(GTK_LEVEL_BAR(widget),
					GTK_LEVEL_BAR_MODE_CONTINUOUS);
		}
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_levelbar_envvar_all_construct(variable *var)
{
	/* Not connected by default */
	return g_strdup("");
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_levelbar_envvar_construct(GtkWidget *widget)
{
	gdouble  value;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	value = gtk_level_bar_get_value(GTK_LEVEL_BAR(widget));

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
	return g_strdup_printf("%g", value);
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_levelbar_fileselect(
	variable *var, const char *name, const char *value)
{
	g_warning("%s(): Fileselect not implemented for this widget.", __func__);
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/

void widget_levelbar_refresh(variable *var)
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

	/* <input> — la sortie de commande est la nouvelle valeur */
	act = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
	while (act) {
		if (input_is_shell_command(act))
			widget_levelbar_input_by_command(var, act + 8);
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_levelbar_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_levelbar_input_by_items(var);

	if (!initialised) {
		if (attributeset_is_avail(var->Attributes, ATTR_LABEL))
			g_warning("%s(): <label> not implemented for this widget.", __func__);
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
			gchar *def = attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT);
			if (def)
				gtk_level_bar_set_value(GTK_LEVEL_BAR(var->Widget),
					g_ascii_strtod(def, NULL));
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

void widget_levelbar_removeselected(variable *var)
{
	g_warning("%s(): Removeselected not implemented for this widget.", __func__);
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_levelbar_save(variable *var)
{
	g_warning("%s(): Save not implemented for this widget.", __func__);
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_levelbar_input_by_command(variable *var, char *command)
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
			gtk_level_bar_set_value(GTK_LEVEL_BAR(var->Widget),
				g_ascii_strtod(line, NULL));
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

static void widget_levelbar_input_by_file(variable *var, char *filename)
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
			gtk_level_bar_set_value(GTK_LEVEL_BAR(var->Widget),
				g_ascii_strtod(line, NULL));
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

static void widget_levelbar_input_by_items(variable *var)
{
	g_warning("%s(): <item> not implemented for this widget.", __func__);
}
