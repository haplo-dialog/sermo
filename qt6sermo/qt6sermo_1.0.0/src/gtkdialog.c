#include "safe_exec.h"
/*
 * gtk3d.c: Starting up the program.
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

/* 
**
** $Id: main.c,v 1.1 2004/11/16 20:47:40 root Exp root $
** $Log: main.c,v $
** Revision 1.1  2004/11/16 20:47:40  root
** Initial revision
**
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <getopt.h>
#ifndef QT6_COMPAT_H
#include <gtk/gtk.h>
#endif

#if HAVE_GLADE_LIB
# include <glade/glade.h>
#endif

#include "config.h"
#include "gtk3d.h"
#include "variables.h"
#include "automaton.h"
#include "gtkdialog_parser.h"
#include "glade_support.h"

#undef DEBUG
#undef WARNING
#include "macros.h"
/*
 * Global variables declared extern in gtk3d.h — defined here once.
 */
gint      function_signals_block = FALSE;
gint      project_space_expand   = -1;
gint      project_space_fill     = -1;
GtkWidget *lastradiowidget        = NULL;
GList     *accel_groups           = NULL;
/* Définis dans widgets.cpp (évite la double définition au link). */
/* GList     *widget_hide_list       = NULL; */
/* GList     *widget_show_list       = NULL; */
gint      window_id               = 0;

/*
 * Program argument variables.
 */
gboolean option_version = FALSE;
gboolean option_debug = FALSE;
gchar *option_input_variable = NULL;
gchar *option_input_file = NULL;
gchar *option_glade_file = NULL;
gchar *option_ignored = NULL;
gchar *option_include_file = NULL;
gchar *option_event_file = NULL;
gchar *option_geometry = NULL;
gchar *option_space_expand = NULL;
gchar *option_space_fill = NULL;
gboolean option_input_stdin = FALSE;
gboolean option_no_warning = FALSE;
gboolean option_print_ir = FALSE;
gboolean option_centering = FALSE;
gchar *option_do_command = NULL;

static gint source = PRG_UNKNOWN;    // Where the program is coming from?
gchar *program_src = NULL;           // The actual program source.
gint charsreaded = 0;                // How much did we red from the source in the memory.
static FILE *sourcefile = NULL;      // The input file handler.
static gchar *program_name = NULL;   // The name of the dialog for messages.

gboolean have_geometry_xy = FALSE;
gboolean have_geometry_dxdy = FALSE;
gint geometry_dx = 0;
gint geometry_dy = 0;
gint geometry_x = 0;
gint geometry_y = 0;

static gboolean 
get_geometry(const char *argument)
{
	g_return_val_if_fail(argument != NULL, FALSE);

	PIP_DEBUG("geometry: '%s'", argument);

	if (sscanf(argument, "%dx%d+%d+%d", &geometry_dx, &geometry_dy, 
				&geometry_x, &geometry_y) == 4){
		PIP_DEBUG("Geometry in form of: %%dx%%d+%%d+%%d");
		have_geometry_dxdy = TRUE;
		have_geometry_xy = TRUE;
		return TRUE;
	}
	
	if (sscanf(argument, "%dx%d", &geometry_dx, &geometry_dy) == 2){
		PIP_DEBUG("Geometry in form of: %%dx%%d");
		have_geometry_dxdy = TRUE;
		return TRUE;
	}
	
	if (sscanf(argument, "+%d+%d", &geometry_x, &geometry_y) == 2){
		PIP_DEBUG("Geometry in form of: +%%dx+%%d");
		have_geometry_xy = TRUE;
		return TRUE;
	}
	
	PIP_WARNING("Unknown geometry form.");
	return FALSE;
}

static void
get_space_value(const char *argument, gint *targetvar)
{
	if ((strcasecmp(argument, "true") == 0) ||
		(strcasecmp(argument, "yes") == 0) ||
		(strcasecmp(argument, "1") == 0)) {
		*targetvar = TRUE;
	} else if ((strcasecmp(argument, "false") == 0) ||
		(strcasecmp(argument, "no") == 0) ||
		(strcasecmp(argument, "0") == 0)) {
		*targetvar = FALSE;
	}
}

/*
 * NOTE: options could be organized into logical groups (input, output, debug)
 * in a future refactoring of the CLI interface.
 */
void
gtkdialog_init(
		int   argc, 
		char *argv[])
{
    GError *error = NULL;
    GOptionContext *context;
    static GOptionEntry entries[] = {
	{ 
		"version", 'v', 
		0, G_OPTION_ARG_NONE, &option_version, 
		"Print version information and exit.", NULL
	},
	{ 
		"debug", 'd', 
		0, G_OPTION_ARG_NONE, &option_debug, 
		"Debug mode prints the processed characters.", NULL
	},
	{ 
		"program", 'p', 
		0, G_OPTION_ARG_STRING, &option_input_variable, 
		"Get the GUI description from the environment.", "variable"
	},
#if HAVE_GLADE_LIB
	{ 
		"glade-xml", 'g', 
		0, G_OPTION_ARG_STRING, &option_glade_file, 
		"Get the GUI description from this Glade file.", "filename"
	},
#else
	{ 
		"glade-xml", 'g', 
		0, G_OPTION_ARG_STRING, &option_ignored, 
		"Ignored, since no Glade library found.", "filename"
	},
#endif
	{ 
		"file", 'f', 
		0, G_OPTION_ARG_STRING, &option_input_file, 
		"Get the GUI description from a regular file.", "filename"
	},
	{ 
		"include", 'i', 
		0, G_OPTION_ARG_STRING, &option_include_file, 
		"Include the given file when executing.", "filename"
	},
	{ 
		"event-driven", 'e', 
		0, G_OPTION_ARG_STRING, &option_event_file, 
		"Execute the file as an event driven program.", "filename"
	},
	{ 
		"stdin", 's', 
		0, G_OPTION_ARG_NONE, &option_input_stdin, 
		"Get the GUI description from standard input.", NULL
	},
	{ 
		"no-warning", 'w', 
		0, G_OPTION_ARG_NONE, &option_no_warning, 
		"Suppress warning messages.", NULL
	},
	{ 
		"geometry", 'G', 
		G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &option_geometry, 
		"The placement and the size of the window.", "[XxY][+W+H]"
	},
	{ 
		"center", 'c', 
		0, G_OPTION_ARG_NONE, &option_centering, 
		"Center the windows on the screen.", NULL
	},
	{ 
		"print-ir", '\0', 
		0, G_OPTION_ARG_NONE, &option_print_ir, 
		"Print the internal representation and exit.", NULL
	},
	{ 
		"space-expand", '\0', 
		0, G_OPTION_ARG_STRING, &option_space_expand, 
		"The \"expand\" state for packing all widgets.", "state"
	},
	{ 
		"space-fill", '\0', 
		0, G_OPTION_ARG_STRING, &option_space_fill, 
		"The \"fill\" state for packing all widgets.", "state"
	},
	{
		"do", '\0', 
		0, G_OPTION_ARG_STRING, &option_do_command, 
		"Execute this shell command after the dialog closes.", "command"
	},
	{
		NULL
	}
    };

#ifdef DEBUG
	{
		gint deb;
		PIP_DEBUG("argc: %d", argc);
		for (deb = 0; deb < argc; ++deb)
			PIP_DEBUG("  argv[%d] = '%s'", deb, argv[deb]);
	}
#endif
	/*
	 * Maybe the command line options are coming from the kernel and it did
	 * not slice it.
	 */
	if (argc > 1 && g_utf8_strchr(argv[1], -1, ' ') != NULL) {
		gint   optionindex;
		gchar *tmp;
		gchar *command_line;

		PIP_DEBUG("Must re-slice the options.");
		/*
		 * The most simple solution is to reassemble the whole command
		 * line and slice it again.
		 */
		tmp = g_strdup(argv[0]);
		for (optionindex = 1; optionindex < argc; ++optionindex) {
			PIP_DEBUG("argv[%d] = '%s'", optionindex, 
					argv[optionindex]);
			command_line = g_strconcat(
					tmp, 
					" ", 
					argv[optionindex], 
					NULL);
			g_free(tmp);
			tmp = command_line;
		}
		/* 
		 * Splitting the command line again.
		 */
		PIP_DEBUG("Full command line: '%s'", command_line);
		argv = g_strsplit_set(command_line, " \t", 256);
		g_free(command_line);
		/*
		 * Re-calculating argc.
		 */
		argc = 0;
		while (argv[argc] != NULL)
			++argc;
#ifdef DEBUG
		{
			gint deb;
			PIP_DEBUG("Re sliced...");
			PIP_DEBUG("argc: %d", argc);
			for (deb = 0; deb < argc; ++deb)
				PIP_DEBUG("  argv[%d] = '%s'", deb, argv[deb]);
		}
#endif
	}
	

	context = g_option_context_new(
"\n"
"Create dialog boxes and windows according to the given dialog description.\n"
"For more information try 'info qt6sermo'."
);

	g_option_context_add_main_entries(context, entries, PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group(FALSE));

	if (!g_option_context_parse(context, &argc, &argv, &error)) 
		g_error("Error parsing command line options: %s", 
				error->message);

	g_option_context_free(context);
}

gint get_program_from_variable(gchar *name)
{
	g_assert(name != NULL);

	PIP_DEBUG("Start.");

	//Redundant: Bug: set_program_name(program_name);
	set_program_name(name);
	program_src = g_strdup(g_getenv(name));

	if (program_src == NULL) 
		g_error(
"Gtkdialog: Could not find the dialog description in the environment "
"variable '%s'.", name);

	source = PRG_MEMORY;
	return 0;
}
	
static gint 
get_program_from_file(char *name)
{
	size_t tmp = 0; int result;
	
	PIP_DEBUG("Start.");
	
	set_program_name("MAIN_WINDOW");
	
	sourcefile = fopen(name, "r");
	if (!sourcefile) 
		g_error("Error opening file '%s': %m", name );
	
	/* Shebang handling: read and discard the first line if it starts with '#'
	 * (e.g. #!/usr/bin/gtk3d), then reopen the file from the beginning
	 * so the parser sees the full XML content.  The gtk3d grammar does
	 * not include a '#!' token, so this pre-pass is required.
	 */
	program_src = NULL;
	result = getline(&program_src, &tmp, sourcefile);
	if (program_src[0] != '#') {
		fclose(sourcefile);
		sourcefile = fopen(name, "r");
		if (!sourcefile) 
			g_error("Error opening file '%s': %m", name );
	}
	g_free(program_src);
	program_src = NULL;
	
	source = PRG_FILE;
	return 0;
}

static gint
get_program_from_stdin(void)
{
	/* Moose / Debian 03_stdin patch: changed from PRG_MEMORY */
	source = PRG_STDIN;
	PIP_DEBUG("Start.");
	return 0;
}

static void 
print_version_exit(int exitcode)
{
	gint extralibs = 0;

	PIP_DEBUG("Start.");

	printf("%s version %s %s (C) 2003-2007 Laszlo Pere, 2011-2012 Thunor\n", 
		PACKAGE_NAME, PACKAGE_VERSION, BUILD_DETAILS);
	printf("Built with additional support for: ");
#if HAVE_GLADE_LIB
	printf("Glade"); extralibs++;
#endif
#if HAVE_VTE
	if (extralibs) printf(", ");
	printf("VTE"); extralibs++;
#endif
	if (!extralibs) printf("none");
	printf(".\n");

	exit(exitcode);
}

int 
getnextchar(void)
{
	int c = EOF;

	//PIP_DEBUG("Start.");

	/*
	 * This is not too GTKish
	 */
	if (source == PRG_MEMORY) {
/*		if (program_src == NULL || charsreaded > strlen(program_src))
			return (EOF);	Redundant: Bug: slowness at start-up */
		c = (int) program_src[charsreaded];
		if (c == '\0')
			c = EOF;
		++charsreaded;
		goto we_have_the_char;
	}
	
	if (source == PRG_STDIN) {
		c = getchar();
		goto we_have_the_char;
	}
	
	if (source == PRG_FILE) {
		c = (int) getc(sourcefile);
		goto we_have_the_char;
	}
	
we_have_the_char:
	/* Raw character debug trace: enabled with --debug flag only. */
	if (option_debug)
		printf("%c", c);

	return c;
}

void
reset_program_source(void) 
{
	charsreaded = 0;

	PIP_DEBUG("");
}

void
set_program_name(gchar *name)
{

	PIP_DEBUG("name: '%s'", name);

	g_free(program_name);
	program_name = g_strdup(name);
}

gchar *
get_program_name(void)
{

	PIP_DEBUG("");

	if (program_name != NULL) 
		return program_name;
	else
/*		return "Unknown"; */
		return "";	/* Thunor: A null string is better */
}

/*
 * We use this function when the program wants to open a new window. This
 * function will search the window description as an environment variable, which
 * is not a perfect solution.
 */
/* Redundant: get_program_from_variable() already does this AND it
 * duplicates the string returned by g_getenv() which is important.
gint
set_program_source(gchar *name)
{
	g_assert(name != NULL);

	PIP_DEBUG("name: '%s'", name);
	/$
	 ** We read the window description from the environment variable.
	 $/
	program_src = (gchar*)g_getenv(name);
	/$
	 ** If we could not find the window we give a message to the user and
	 ** exit.
	 $/
	if (program_src == NULL) 
		g_error("%s(): Could not find window: '%s'.", __func__, name);
	
	source = PRG_MEMORY;
	charsreaded = 0;
	set_program_name(name);
} */

/* --do : execute a shell command after the dialog closes, with widget values
 * already exported to the environment. Registered via atexit() because the
 * program has several direct exits (a widget exit: action, last-window close)
 * that never return through main(). Goes through safe_system() -- same path as
 * everything else: direct exec() when the command has no metacharacter, logged
 * /bin/sh -c fallback otherwise, refused with HAPLO_NO_SHELL_FALLBACK=1. No new
 * surface: the command comes from the caller own command line. */
static void
run_do_command_at_exit(void)
{
	gint rc;

	if (option_do_command == NULL)
		return;

	variables_export_all();
	fflush(NULL);
	rc = safe_system(option_do_command);
	if (rc != 0)
		g_warning("--do: command exited with code %d", rc);
}

int 
main(int argc, char *argv[])
{
	/* Initialise global variables */
	GTKD_FUNCTION_SIGNALS_RESET;
	lastradiowidget = NULL;
	accel_groups = NULL;
	widget_hide_list = widget_show_list = NULL;
	window_id = 0;

	setlocale(LC_ALL, "");
	/*
	 * 
	 */
	/* g_thread_init / gdk_threads_init removed — no-op since GLib/GDK 3.x */
	/*
	 * Processing command line arguments.
	 */
	gtkdialog_init(argc, argv);

	if (option_do_command != NULL)
		atexit(run_do_command_at_exit);
	
	if (option_version)
		print_version_exit(EXIT_SUCCESS);
	
	if (option_geometry != NULL) 
		get_geometry(option_geometry);

	/* Thunor: I've added these to control/override the widget
	 * packing expand and fill states at the project level */
	project_space_expand = -1;
	if (option_space_expand)
		get_space_value(option_space_expand, &project_space_expand);
	project_space_fill = -1;
	if (option_space_fill)
		get_space_value(option_space_fill, &project_space_fill);

	if (option_input_stdin) {
		get_program_from_stdin();
		goto gtkdialog_initialized;
	}
	/*
	 * If we have an event driven program we recall the gtk3d with
	 * a source file included into the environment.
	 */
#ifdef DEBUG
	fprintf(stderr, "%s(): argv[0]='%s'\n", __func__, argv[0]);
#endif
	if (option_event_file != NULL) {
		gchar *command;
		command = g_strdup_printf(
				/* Debian 01_bashism patch: use dot rather than source.
				"source %s; " */
				". %s; "
				"%s %s%s%s%s%s%s -i %s",
				option_event_file,
				argv[0],
				option_debug ? "-d " : "",
				option_no_warning ? "-w" : "",
				option_centering ? "-c " : "",
				option_print_ir ? "--print-ir " : "",
				option_input_variable ? "--program=" : "",
				option_input_variable ? option_input_variable : "",
				option_event_file);
		exit(safe_system(command));
	}
	
	if (option_input_file != NULL) {
		get_program_from_file(option_input_file);
		goto gtkdialog_initialized;
	}
	
	if (option_input_variable != NULL && option_glade_file == NULL) { 
		get_program_from_variable(option_input_variable);
		goto gtkdialog_initialized;
	}
	
#if HAVE_GLADE_LIB
	if (option_glade_file == NULL) 
		get_program_from_variable("MAIN_DIALOG");
#else
		get_program_from_variable("MAIN_DIALOG");
#endif

gtkdialog_initialized:
	gtk_init(&argc, &argv);
	
#if HAVE_GLADE_LIB
	if (option_glade_file != NULL) {
		run_program_by_glade(option_glade_file, option_input_variable);
		exit(EXIT_SUCCESS);
	}
#endif
	
	gtkdialog_parse();
}
