/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_calendar.c:
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
 * GtkCalendar — a full month calendar widget for date selection.
 *
 * XML usage:
 *   <calendar>
 *     <variable>SELECTED_DATE</variable>
 *     <default>2026-05-21</default>
 *     <action>echo "Date: $SELECTED_DATE"</action>
 *   </calendar>
 *
 * Environment variable: "YYYY-MM-DD" (ISO 8601 format).
 * Signals:
 *   day-selected        — fires when user clicks a day
 *   prev-month / next-month / prev-year / next-year — navigation
 *   (⛔ « month-changed » et « day-selected-double-click » n'existent PAS en
 *    GTK 4 : g_signal_lookup rend 0 pour les deux.)
 *
 * The <default> attribute accepts "YYYY-MM-DD" format.
 * Month in GtkCalendar is 0-based (January = 0), adjusted internally.
 */

/* Includes */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#include <time.h>
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
static void widget_calendar_input_by_command(variable *var, char *command);
static void widget_calendar_input_by_file(variable *var, char *filename);
static void widget_calendar_input_by_items(variable *var);

/* Notes:
 * gtk_calendar_get_date() fills year, month (0–11), day.
 * We export "YYYY-MM-DD" with month+1.
 * gtk_calendar_select_month(cal, month, year) — month is 0-based.
 * gtk_calendar_select_day(cal, day) — day is 1-based.
 * Setting a date from a "YYYY-MM-DD" string requires parsing.
 */


/* ─────────────────────────────────────────────────────────────────────────
 * Remplaçants GTK 4 des deux macros mortes (déclarés dans gtk4-compat.h).
 *
 * ⚠️ gtk_calendar_get_date et gtk_calendar_select_day sont désormais des
 * MACROS : on entoure leur nom de parenthèses pour appeler la VRAIE fonction
 * GTK et empêcher l'expansion.
 * ───────────────────────────────────────────────────────────────────────── */

static void hp_calendar_apply(GtkCalendar *calendar, gint year, gint month1, gint day)
{
	GDateTime *dt;
	gint       jours_du_mois;

	year   = CLAMP(year, 1, 9999);
	month1 = CLAMP(month1, 1, 12);
	jours_du_mois = (gint)g_date_get_days_in_month((GDateMonth)month1, (GDateYear)year);
	day = CLAMP(day, 1, jours_du_mois);

	/* Midi et non minuit : dans certains fuseaux, minuit n'existe pas le jour
	 * d'un changement d'heure et g_date_time_new_local() décalerait la date. */
	dt = g_date_time_new_local(year, month1, day, 12, 0, 0);
	if (dt == NULL) {
		g_warning("%s(): date invalide %04d-%02d-%02d.", __func__, year, month1, day);
		return;		/* gtk_calendar_set_date asserte date != NULL */
	}

#if GTK_CHECK_VERSION(4, 20, 0)
	gtk_calendar_set_date(calendar, dt);
#else
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	(gtk_calendar_select_day)(calendar, dt);
	G_GNUC_END_IGNORE_DEPRECATIONS
#endif
	g_date_time_unref(dt);
}

static void hp_calendar_current(GtkCalendar *calendar, gint *year, gint *month1, gint *day)
{
	GDateTime *cur = (gtk_calendar_get_date)(calendar);

	if (cur) {
		*year   = g_date_time_get_year(cur);
		*month1 = g_date_time_get_month(cur);
		*day    = g_date_time_get_day_of_month(cur);
		g_date_time_unref(cur);
	} else {
		*year = 1970; *month1 = 1; *day = 1;
	}
}

void hp_calendar_set_date(GtkWidget *calendar, gint year, gint month0, gint day)
{
	if (!GTK_IS_CALENDAR(calendar)) return;
	hp_calendar_apply(GTK_CALENDAR(calendar), year, month0 + 1, day);
}

void hp_calendar_select_month(GtkWidget *calendar, gint month0, gint year)
{
	gint y, m1, d;

	if (!GTK_IS_CALENDAR(calendar)) return;
	hp_calendar_current(GTK_CALENDAR(calendar), &y, &m1, &d);
	hp_calendar_apply(GTK_CALENDAR(calendar), year, month0 + 1, d);
}

void hp_calendar_select_day(GtkWidget *calendar, gint day)
{
	gint y, m1, d;

	if (!GTK_IS_CALENDAR(calendar)) return;
	hp_calendar_current(GTK_CALENDAR(calendar), &y, &m1, &d);
	hp_calendar_apply(GTK_CALENDAR(calendar), y, m1, day);
}

/* Helper: set calendar date from "YYYY-MM-DD" string */
static void calendar_set_date_from_string(GtkCalendar *calendar,
	const gchar *datestr)
{
	gint year = 0, month = 0, day = 0;
	if (datestr && sscanf(datestr, "%d-%d-%d", &year, &month, &day) == 3) {
		/* Un SEUL appel : régler mois puis jour séparément échoue quand le
		 * jour courant n'existe pas dans le mois visé (31 → février). */
		hp_calendar_set_date(GTK_WIDGET(calendar),
			year, CLAMP(month - 1, 0, 11), day);
	}
}

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_calendar_clear(variable *var)
{
	time_t     now;
	struct tm *tm_now;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Reset to today's date */
	time(&now);
	tm_now = localtime(&now);
	gtk_calendar_select_month(GTK_CALENDAR(var->Widget),
		(guint)tm_now->tm_mon, (guint)(tm_now->tm_year + 1900));
	gtk_calendar_select_day(GTK_CALENDAR(var->Widget),
		(guint)tm_now->tm_mday);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_calendar_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget        *widget;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget = gtk_calendar_new();

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_calendar_envvar_all_construct(variable *var)
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

gchar *widget_calendar_envvar_construct(GtkWidget *widget)
{
	gchar            *string = NULL;
	guint             year, month, day;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	gtk_calendar_get_date(GTK_CALENDAR(widget), &year, &month, &day);
	/* month is 0-based in GtkCalendar — add 1 for ISO 8601 */
	string = g_strdup_printf("%04u-%02u-%02u", year, month + 1, day);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_calendar_fileselect(
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

void widget_calendar_refresh(variable *var)
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
			widget_calendar_input_by_command(var, act + 8);
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_calendar_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_calendar_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
			value = attributeset_get_first(&element, var->Attributes,
				ATTR_DEFAULT);
			calendar_set_date_from_string(GTK_CALENDAR(var->Widget), value);
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
		g_signal_connect(G_OBJECT(var->Widget), "day-selected",
			G_CALLBACK(on_any_widget_changed_event), (gpointer)var->Attributes);
		/* GTK 4 : « month-changed » n'existe plus sur GtkCalendar — mesuré,
		 * g_signal_lookup rend 0 et g_signal_connect émettait
		 * « signal 'month-changed' is invalid for instance … GtkCalendar »
		 * quatre fois par exécution. Remplaçants nommés, MÊME arité
		 * void (GtkCalendar *, gpointer), donc aucun adaptateur. */
		g_signal_connect(G_OBJECT(var->Widget), "prev-month",
			G_CALLBACK(on_any_widget_changed_event), (gpointer)var->Attributes);
		g_signal_connect(G_OBJECT(var->Widget), "next-month",
			G_CALLBACK(on_any_widget_changed_event), (gpointer)var->Attributes);
		g_signal_connect(G_OBJECT(var->Widget), "prev-year",
			G_CALLBACK(on_any_widget_changed_event), (gpointer)var->Attributes);
		g_signal_connect(G_OBJECT(var->Widget), "next-year",
			G_CALLBACK(on_any_widget_changed_event), (gpointer)var->Attributes);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_calendar_removeselected(variable *var)
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

void widget_calendar_save(variable *var)
{
	FILE             *outfile;
	GList            *element;
	gchar            *act;
	gchar            *filename = NULL;
	gchar            *value;
	guint             year, month, day;

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
			gtk_calendar_get_date(GTK_CALENDAR(var->Widget),
				&year, &month, &day);
			value = g_strdup_printf("%04u-%02u-%02u", year, month + 1, day);
			fprintf(outfile, "%s", value);
			g_free(value);
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

static void widget_calendar_input_by_command(variable *var, char *command)
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
			calendar_set_date_from_string(GTK_CALENDAR(var->Widget), line);
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

static void widget_calendar_input_by_file(variable *var, char *filename)
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
			calendar_set_date_from_string(GTK_CALENDAR(var->Widget), line);
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

static void widget_calendar_input_by_items(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning("%s(): <item> not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}
