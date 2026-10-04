/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_progressbar.c: 
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
 * Copyright (C) 2011-2012  Thunor <thunorsif@hotmail.com>
 * Copyright (C) 2026  haplo-dialog <devel@haplo-dialog.fr> (GTK3 port, security)
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
#include "sermo_input.h"

/* Defines */
//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

/* Local variables */
struct _progr_descr {
	GtkWidget    *widget;
	gdouble       fraction;
	const gchar  *shell_command;
	FILE         *pipe;
	GThread      *thread;
	AttributeSet *Attr;
	/* La mise à jour en attente de la boucle principale, sous
	 * G_LOCK(any_progress_bar) : voir widget_progressbar_apply(). */
	gboolean      a_fraction;       /* une fraction attend */
	gdouble       fraction_attente;
	gchar        *texte_attente;    /* le dernier texte, NULL sinon */
	guint         actions_attente;  /* passages à 100 pas encore servis */
	gboolean      idle_prevu;       /* un widget_progressbar_apply() est prévu */
};
typedef struct _progr_descr progr_descr;

G_LOCK_DEFINE_STATIC(any_progress_bar);

/* Local function prototypes, located at file bottom */
static void widget_progressbar_input_by_command(variable *var, char *command);
static void widget_progressbar_input_by_file(variable *var, char *filename);
static void widget_progressbar_input_by_items(variable *var);
static void widget_progressbar_perform_actions(progr_descr *descr);
static gpointer widget_progressbar_thread_entry(progr_descr *descr);
void widget_progressbar_descriptor_destroy_notify(progr_descr *descr);
void widget_progressbar_realized_callback(GtkWidget *widget, AttributeSet *Attr);

/* Notes:
 * This widget is interesting! The "realized" signal is being used to
 * execute the contents of the input directive and if that terminates
 * then it's game over for this widget because you can't refresh it */

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_progressbar_clear(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	fprintf(stderr, "%s(): Clear not implemented for this widget.\n", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_progressbar_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GList            *element;
	GtkWidget        *widget;
	gchar            *text;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Thunor: This is all original code moved across when refactoring */
	widget = gtk_progress_bar_new();

	/* Thunor: This is all original code moved across when refactoring */
	if (attributeset_is_avail(Attr, ATTR_LABEL)) {
		text = attributeset_get_first(&element, Attr, ATTR_LABEL);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(widget), text);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_progressbar_envvar_all_construct(variable *var)
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

gchar *widget_progressbar_envvar_construct(GtkWidget *widget)
{
	gchar            *string = NULL;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	string = g_strdup("");

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_progressbar_fileselect(
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

void widget_progressbar_refresh(variable *var)
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
			widget_progressbar_input_by_command(var, act + 8);
		/* input file stock = "File:", input file = "File:/path/to/file" */
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			widget_progressbar_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_progressbar_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT))
			fprintf(stderr, "%s(): <default> not implemented for this widget.\n",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_HEIGHT))
			fprintf(stderr, "%s(): <height> not implemented for this widget.\n",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_WIDTH))
			fprintf(stderr, "%s(): <width> not implemented for this widget.\n",
				__func__);
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||	/* Deprecated */
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);

		/* Connect signals */
		/* Thunor: This is all original code moved across when refactoring */
		/* We start the input command in a separate thread when the
		 * widget gets realized */
		g_signal_connect(G_OBJECT(var->Widget), "realize",
			G_CALLBACK(widget_progressbar_realized_callback),
			(gpointer)var->Attributes);

	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_progressbar_removeselected(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	fprintf(stderr, "%s(): Removeselected not implemented for this widget.\n",
		__func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_progressbar_save(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	fprintf(stderr, "%s(): Save not implemented for this widget.\n", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_progressbar_input_by_command(variable *var, char *command)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* <input> is handled elsewhere for this widget */

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by File                                                       *
 ***********************************************************************/

static void widget_progressbar_input_by_file(variable *var, char *filename)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	fprintf(stderr, "%s(): <input file> not implemented for this widget.\n", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Items                                                      *
 ***********************************************************************/

static void widget_progressbar_input_by_items(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	fprintf(stderr, "%s(): <item> not implemented for this widget.\n", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Some stuff to handle progress bars                                  *
 ***********************************************************************/
/* Thunor: This simply executed all actions when the progressbar reached
 * 100 so I've fixed that by routing it through gtk4sermo's normal signal
 * handling system and the default signal is now "time-out" */

static void widget_progressbar_perform_actions(progr_descr *descr)
{

	widget_signal_executor(descr->widget, descr->Attr, "time-out");

/* Redundant.
	GList *element;
	gchar *command;
	gchar *type;

	command = attributeset_get_first(&element, descr->Attr, ATTR_ACTION);
	while (command != NULL){
		type = attributeset_get_this_tagattr(&element, descr->Attr,
			ATTR_ACTION, "type");
		execute_action(descr->widget, command, type);
next_command:   
		command = attributeset_get_next(&element, descr->Attr, ATTR_ACTION);
	} */
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* Thunor: This is all original code moved across when refactoring */

/* haplo-dialog 2026 : la mise a jour repart vers la boucle principale.
 *
 * Le code d'origine touchait GTK depuis ce thread de lecture, et y faisait
 * meme tourner la boucle principale (gtk_main_iteration_do). GTK n'est pas
 * reentrant : deux boucles concurrentes allouent et dispatchent en meme
 * temps, et le processus meurt au hasard. gdk_threads_enter/leave, qui
 * etait cense proteger cette region, ne fait plus rien depuis GTK 3.6.
 *
 * Le thread se contente desormais de lire le tuyau ; tout ce qui touche a
 * GTK est confie a la boucle principale par g_idle_add().
 *
 * 2.7.3 : UNE mise a jour en attente, pas une par ligne. La 2.7.2 confiait
 * chaque ligne lue a son propre g_idle_add() : une commande rapide et sans fin
 * (`yes 50`) remplissait la file plus vite que la boucle ne la vidait, et le
 * processus grossissait de 70 Mo par seconde (mesure le 2026-09-24). Le thread
 * depose desormais la derniere fraction et le dernier texte dans le
 * descripteur, et ne prevoit un idle que s'il n'y en a pas deja un. Seul
 * l'affichage final compte ; les passages a 100 sont comptes, pour que les
 * actions tournent autant de fois qu'avant. */
static gboolean widget_progressbar_apply(gpointer data)
{
	progr_descr *descr = (progr_descr *)data;
	gboolean     alive;
	gchar       *text;
	guint        actions;

	G_LOCK(any_progress_bar);
	descr->idle_prevu = FALSE;
	text = descr->texte_attente;
	descr->texte_attente = NULL;
	actions = descr->actions_attente;
	descr->actions_attente = 0;
	alive = (descr->widget != NULL);
	if (alive) {
		if (descr->a_fraction)
			gtk_progress_bar_set_fraction(
				GTK_PROGRESS_BAR(descr->widget),
				descr->fraction_attente);
		if (text != NULL)
			gtk_progress_bar_set_text(
				GTK_PROGRESS_BAR(descr->widget),
				text);
	}
	descr->a_fraction = FALSE;
	G_UNLOCK(any_progress_bar);

	if (alive)
		for (; actions > 0; actions--)
			widget_progressbar_perform_actions(descr);

	g_free(text);
	return G_SOURCE_REMOVE;
}

static gpointer widget_progressbar_thread_entry(progr_descr *descr)
{
	char      oneline[512];
	gboolean  actions_performed = FALSE;
	long int  ival;
	char     *end;
	gint length;
	gboolean  gone;

	while (fgets(oneline, 512, descr->pipe) != NULL) {
		/* Une ligne qui commence par un octet nul a une longueur de zéro :
		 * sans la garde, on lirait oneline[-1], hors du tampon. */
		length = (gint)strlen(oneline) - 1;
		if (length >= 0 && oneline[length] == '\n')
			oneline[length] = '\0';

		ival = strtol(oneline, &end, 0);
		descr->fraction =  ival / 100.0;
		if (descr->fraction > 1.0)
			descr->fraction = 1.0;

		if (ival != 100)
			actions_performed = FALSE;

		G_LOCK(any_progress_bar);
		gone = (descr->widget == NULL);
		if (!gone) {
			if (end == oneline) {
				/* Pas un nombre : c'est le texte de la barre. */
				g_free(descr->texte_attente);
				descr->texte_attente = g_strdup(oneline);
			} else {
				descr->fraction_attente = descr->fraction;
				descr->a_fraction = TRUE;
			}
			if (ival == 100 && !actions_performed) {
				descr->actions_attente++;
				actions_performed = TRUE;
			}
			if (!descr->idle_prevu) {
				descr->idle_prevu = TRUE;
				g_idle_add(widget_progressbar_apply, descr);
			}
		}
		G_UNLOCK(any_progress_bar);
		if (gone)
			break;
	}

	fclose(descr->pipe);
	return NULL;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* Thunor: This is all original code moved across when refactoring */

void widget_progressbar_descriptor_destroy_notify(progr_descr *descr)
{
	g_assert(descr != NULL);
	G_LOCK(any_progress_bar);
	descr->widget = NULL;
	G_UNLOCK(any_progress_bar);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* Thunor: This is all original code moved across when refactoring */
/* In this function we create a descriptor and start up a thread to
 * handle the input of the progress bar */

void widget_progressbar_realized_callback(GtkWidget *widget, AttributeSet *Attr)
{
	GList *element;
	progr_descr *descr;
	gchar *input;

#ifdef DEBUG
	g_message("%s(%p, %p);", __func__, widget, Attr);
#endif	
	g_assert(GTK_IS_WIDGET(widget) && Attr != NULL);
	/*
	 * If there is no input, we can return.
	 */
	if (!attributeset_is_avail(Attr, ATTR_INPUT))
		return;
	/*
	 * Creating the descriptor from the first input.
	 */
	input = attributeset_get_first(&element, Attr, ATTR_INPUT);
	descr = g_new0(progr_descr, 1);
	descr->Attr = Attr;
	descr->widget = widget;
	descr->shell_command = input_get_shell_command(input);
	/* La barre lit une ligne a la fois et n'accumule rien : elle suit la
	 * commande jusqu'au bout, sans la limite des <input> (sermo_input.h). */
	descr->pipe = sermo_input_sans_limite(
			widget_opencommand(descr->shell_command));
	/* Commande refusee (SERMO_ALLOWED_CMDS) ou lancement impossible :
	 * widget_opencommand() l'a deja signale. La 2.7.2 lancait quand meme
	 * le fil de lecture, qui tombait sur fgets(NULL) — erreur de
	 * segmentation (mesure le 2026-09-24). La barre reste vide. */
	if (descr->pipe == NULL) {
		g_free(descr);
		return;
	}
	/*
	 * When the widget gets destroyed we call this function to prevent
	 * further reading the pipe and setting the destroyed progress bar.
	 */
	g_object_set_data_full(G_OBJECT(widget),
			"descriptor",
			descr, 
			(GDestroyNotify)widget_progressbar_descriptor_destroy_notify);
	/*
	 * Now we can fire up the reader thread.
	 */
	descr->thread = g_thread_create(
			(GThreadFunc) widget_progressbar_thread_entry, 
			descr, FALSE, NULL);
}
