/*
 * widget_terminal.c: 
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
#include "gtk3d.h"
#include "widget_terminal.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "tag_attributes.h"
#if HAVE_VTE
#include <vte/vte.h>
#endif

/* Defines */
//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

#define VTE_WARNING "The terminal (VteTerminal) widget requires \
a version of gtk3sermo built with libvte."

/* Local function prototypes, located at file bottom */
static void widget_terminal_input_by_command(variable *var, char *command);
static void widget_terminal_input_by_file(variable *var, char *filename);
static void widget_terminal_input_by_items(variable *var);

/* Notes: */

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_terminal_clear(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

#if HAVE_VTE
	/* This won't result in child-exited being emitted */
	vte_terminal_reset(VTE_TERMINAL(var->Widget), TRUE, TRUE);
	widget_terminal_fork_command(var->Widget, var->widget_tag_attr);
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/
GtkWidget *widget_terminal_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget        *widget;
#if HAVE_VTE
	GdkRGBA          color;
	GList            *element;
	gchar             tagattribute[256];
	gchar            *value;
	gint              width = -1, height = -1;
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

#if HAVE_VTE
	/* Read declared directives */
	if (attributeset_is_avail(Attr, ATTR_WIDTH))
		width = atoi(attributeset_get_first(&element, Attr, ATTR_WIDTH));
	if (attributeset_is_avail(Attr, ATTR_HEIGHT))
		height = atoi(attributeset_get_first(&element, Attr, ATTR_HEIGHT));

	widget = vte_terminal_new();
	
	/* Until I added this, none of the functions that set various text
	 * foreground and background colours worked so I'm presuming that
	 * creating a new VteTerminal doesn't create a full palette */
	vte_terminal_set_default_colors(VTE_TERMINAL(widget));

	if (attr) {
		/* The gtk property "font-desc" requires a pointer to a
		 * PangoFontDescription but the application developer can't pass
		 * anything other than a string here, so we'll convert it using
		 * a dedicated gtk function and then kill the tag attribute else
		 * widget_set_tag_attributes() will try to set it later */
		g_strlcpy(tagattribute, "font-desc", sizeof(tagattribute));
		if ((value = get_tag_attribute(attr, tagattribute))) {
			vte_terminal_set_font(VTE_TERMINAL(widget), pango_font_description_from_string(value));
			kill_tag_attribute(attr, tagattribute);
		}

		/* Again, "background-tint-color" requires a pointer to a
		 * GdkRGBA struct but we can convert a string like "#ff00ff" */
		g_strlcpy(tagattribute, "background-tint-color", sizeof(tagattribute));
		if ((value = get_tag_attribute(attr, tagattribute))) {
			/* Parse the RGB value to create the necessary GdkRGBA.
			 * This function doesn't like trailing whitespace so it
			 * needs to be stripped first with g_strstrip() */ 
			if (gdk_rgba_parse(&color, g_strstrip(value))) {
#ifdef DEBUG_CONTENT
				fprintf(stderr, "%s:() valid colour found\n", __func__);
#endif
				vte_terminal_set_color_background(VTE_TERMINAL(widget), &color);
			}
			kill_tag_attribute(attr, tagattribute);
		}

		/* Get custom tag attribute "font-name" */
		if ((value = get_tag_attribute(attr, "font-name"))) {
			vte_terminal_set_font(VTE_TERMINAL(widget), pango_font_description_from_string(value));
		}

		/* Get custom tag attribute "text-background-color" */
		if ((value = get_tag_attribute(attr, "text-background-color"))) {
			/* Parse the RGB value to create the necessary GdkRGBA.
			 * This function doesn't like trailing whitespace so it
			 * needs to be stripped first with g_strstrip() */ 
			if (gdk_rgba_parse(&color, g_strstrip(value))) {
#ifdef DEBUG_CONTENT
				fprintf(stderr, "%s:() valid colour found\n", __func__);
#endif
				vte_terminal_set_color_background(VTE_TERMINAL(widget), &color);
			}
		}

		/* Get custom tag attribute "text-foreground-color" */
		if ((value = get_tag_attribute(attr, "text-foreground-color"))) {
			/* Parse the RGB value to create the necessary GdkRGBA.
			 * This function doesn't like trailing whitespace so it
			 * needs to be stripped first with g_strstrip() */ 
			if (gdk_rgba_parse(&color, g_strstrip(value))) {
#ifdef DEBUG_CONTENT
				fprintf(stderr, "%s:() valid colour found\n", __func__);
#endif
				vte_terminal_set_color_foreground(VTE_TERMINAL(widget), &color);
			}
		}

		/* Get custom tag attribute "bold-foreground-color" */
		if ((value = get_tag_attribute(attr, "bold-foreground-color"))) {
			/* Parse the RGB value to create the necessary GdkRGBA.
			 * This function doesn't like trailing whitespace so it
			 * needs to be stripped first with g_strstrip() */ 
			if (gdk_rgba_parse(&color, g_strstrip(value))) {
#ifdef DEBUG_CONTENT
				fprintf(stderr, "%s:() valid colour found\n", __func__);
#endif
				vte_terminal_set_color_bold(VTE_TERMINAL(widget), &color);
			}
		}

		/* Get custom tag attribute "dim-foreground-color" */
		if ((value = get_tag_attribute(attr, "dim-foreground-color"))) {
			/* Parse the RGB value to create the necessary GdkRGBA.
			 * This function doesn't like trailing whitespace so it
			 * needs to be stripped first with g_strstrip() */ 
			if (gdk_rgba_parse(&color, g_strstrip(value))) {
#ifdef DEBUG_CONTENT
				fprintf(stderr, "%s:() valid colour found\n", __func__);
#endif
				/* vte_terminal_set_color_dim() removed in VTE 0.38:
				 * dim colour is computed automatically by VTE. */
				(void)color;
			}
		}

		/* Get custom tag attribute "cursor-background-color" */
		if ((value = get_tag_attribute(attr, "cursor-background-color"))) {
			/* Parse the RGB value to create the necessary GdkRGBA.
			 * This function doesn't like trailing whitespace so it
			 * needs to be stripped first with g_strstrip() */ 
			if (gdk_rgba_parse(&color, g_strstrip(value))) {
#ifdef DEBUG_CONTENT
				fprintf(stderr, "%s:() valid colour found\n", __func__);
#endif
				vte_terminal_set_color_cursor(VTE_TERMINAL(widget), &color);
			}
		}

		/* Get custom tag attribute "highlight-background-color" */
		if ((value = get_tag_attribute(attr, "highlight-background-color"))) {
			/* Parse the RGB value to create the necessary GdkRGBA.
			 * This function doesn't like trailing whitespace so it
			 * needs to be stripped first with g_strstrip() */ 
			if (gdk_rgba_parse(&color, g_strstrip(value))) {
#ifdef DEBUG_CONTENT
				fprintf(stderr, "%s:() valid colour found\n", __func__);
#endif
				vte_terminal_set_color_highlight(VTE_TERMINAL(widget), &color);
			}
		}
	}

	/* Set width and height if both supplied */
	if (width != -1 && height != -1)
		vte_terminal_set_size(VTE_TERMINAL(widget), width, height);

	/* And off we go... */
	widget_terminal_fork_command(widget, attr);
#else
	/* If libvte support is missing then create a label instead */
	widget = gtk_label_new(VTE_WARNING);
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Fork Command                                                        *
 ***********************************************************************/

void widget_terminal_fork_command(GtkWidget *widget, tag_attr *attr)
{
#if HAVE_VTE
	static gchar     *argv0 = "/bin/sh";
	/* [128] is filled by the argvN/envvN loop below (indices 0..127); the
	 * extra slot [128] is a guaranteed NULL terminator for g_spawn/vte, even
	 * when all 128 argvN attributes are set (CWE-170). */
	gchar            *argv[129], *envv[129];
	gchar             tagattribute[256];
	gchar            *value;
	gchar            *working_directory = NULL;
	gint              count;
#if VTE_CHECK_VERSION(0,26,0)
	gboolean          retval;
	GError           *error = NULL;
	GPid              pid;
#else
	pid_t             pid;
#endif
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

#if HAVE_VTE
	/* Initialise strings */
	for (count = 0; count < 129; count++) {
		argv[count] = NULL;
		envv[count] = NULL;
	}
	argv[0] = argv0;	/* Set a default command otherwise it segfaults */

	if (attr) {
		/* The "current-directory-uri" can only be set when we fork a
		 * command (there's no function for it) so we set it now */
		if ((value = get_tag_attribute(attr, "current-directory-uri")))
			working_directory = value;

		/* Get custom tag attributes argv and envv */
		for (count = 0; count < 128; count++) {
			g_snprintf(tagattribute, sizeof(tagattribute), "argv%i", count);
			if ((value = get_tag_attribute(attr, tagattribute)))
				argv[count] = value;
#ifdef DEBUG_CONTENT
			fprintf(stderr, "%s:() %s=%s\n", __func__, tagattribute, value);
#endif
			g_snprintf(tagattribute, sizeof(tagattribute), "envv%i", count);
			if ((value = get_tag_attribute(attr, tagattribute)))
				envv[count] = value;
#ifdef DEBUG_CONTENT
			fprintf(stderr, "%s:() %s=%s\n", __func__, tagattribute, value);
#endif
		}
	}

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s:() working_directory=%s\n", __func__, working_directory);
	fprintf(stderr, "%s:() argv=%p *argv=%s argv[0]=%s argv[1]=%s\n",
		__func__, argv, *argv, argv[0], argv[1]);
	fprintf(stderr, "%s:() envv=%p *envv=%s envv[0]=%s envv[1]=%s\n",
		__func__, envv, *envv, envv[0], envv[1]);
#endif

#if VTE_CHECK_VERSION(0,26,0)
	/* vte_terminal_spawn_sync() — VTE 2.91 (GTK3) signature:
	 *   terminal, pty_flags, working_directory, argv, envv, spawn_flags,
	 *   child_setup, child_setup_data, child_pid, cancellable, error */
	retval = vte_terminal_spawn_sync(VTE_TERMINAL(widget),
		VTE_PTY_DEFAULT,
		working_directory,
		argv,
		envv,
		G_SPAWN_SEARCH_PATH,
		NULL,            /* child_setup */
		NULL,            /* child_setup_data */
		&pid,
		NULL,            /* cancellable */
		&error);
	if (!retval) {
		g_warning("%s(): vte_terminal_spawn_sync(): %s", __func__, error ? error->message : "unknown error");
		if (error) g_error_free(error);
	}
#else
	/* VTE < 0.26 not supported in this GTK3 port — build with vte-2.91 */
	g_warning("%s(): VTE < 0.26 not supported by gtk3sermo GTK3 port", __func__);
	pid = 0;
#endif

	/* Store "pid" as a piece of widget data (recreated if exists)  */
	g_object_set_data(G_OBJECT(widget), "_pid", GINT_TO_POINTER(pid));

#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_terminal_envvar_all_construct(variable *var)
{
	gchar            *string;

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

	return NULL;  /* stub: aucune variable agrégée à exporter */
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_terminal_envvar_construct(GtkWidget *widget)
{
#if HAVE_VTE
	gchar             envvar[32];
#endif
	gchar            *string;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

#if HAVE_VTE
	g_snprintf(envvar, sizeof(envvar), "%i", GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_pid")));
	string = g_strdup(envvar);
#else
	string = g_strdup("");
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_terminal_fileselect(
	variable *var, const char *name, const char *value)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning( "%s(): Fileselect not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/
void widget_terminal_refresh(variable *var)
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
			widget_terminal_input_by_command(var, act + 8);
		/* input file stock = "File:", input file = "File:/path/to/file" */
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised) {
				/* Check for file-monitor and create if requested */
				widget_file_monitor_try_create(var, act + 5);
			}
			widget_terminal_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_terminal_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
		if (attributeset_is_avail(var->Attributes, ATTR_LABEL))
			g_warning( "%s(): <label> not implemented for this widget.",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT))
			g_warning( "%s(): <default> not implemented for this widget.",
				__func__);
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||	/* Deprecated */
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);

		/* Connect signals */
#if HAVE_VTE
		g_signal_connect(G_OBJECT(var->Widget), "child-exited",
			G_CALLBACK(on_any_widget_child_exited_event), (gpointer)var->Attributes);
#endif

	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_terminal_removeselected(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning( "%s(): Removeselected not implemented for this widget.",
		__func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_terminal_save(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning( "%s(): Save not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_terminal_input_by_command(variable *var, char *command)
{
#if HAVE_VTE
	FILE             *infile;
	GString          *text = g_string_sized_new(512);
	gchar             line[512];
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s(): command: '%s'\n", __func__, command);
#endif

#if HAVE_VTE
	/* Opening pipe for reading... */
	if ((infile = widget_opencommand(command))) {
		/* Read the file one line at a time */
		while (fgets(line, 512, infile)) {
			g_string_append(text, line);
		}

		vte_terminal_feed_child(VTE_TERMINAL(var->Widget), text->str, text->len);

		g_string_free(text, TRUE);
		/* Close the file */
		fclose(infile);    /* safe_popen() uses fdopen() → fclose(), not pclose() */
	} else {
		g_warning("%s(): Couldn't open '%s' for reading.", __func__,
			command);
	}
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by File                                                       *
 ***********************************************************************/

static void widget_terminal_input_by_file(variable *var, char *filename)
{
#if HAVE_VTE
	FILE             *infile;
	GString          *text = g_string_sized_new(512);
	gchar             line[512];
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

#if HAVE_VTE
	if ((infile = fopen(filename, "r"))) {
		/* Read the file one line at a time */
		while (fgets(line, 512, infile)) {
			g_string_append(text, line);
		}

		vte_terminal_feed_child(VTE_TERMINAL(var->Widget), text->str, text->len);

		g_string_free(text, TRUE);
		/* Close the file */
		fclose(infile);
	} else {
		g_warning("%s(): Couldn't open '%s' for reading.", __func__,
			filename);
	}
#endif

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Items                                                      *
 ***********************************************************************/

static void widget_terminal_input_by_items(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning( "%s(): <item> not implemented for this widget.", __func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}
