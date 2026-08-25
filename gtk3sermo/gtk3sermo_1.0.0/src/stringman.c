#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/*
 * stringman.c: Special string maniplutations.
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  Lï¿½szlï¿½ Pere <pipas@linux.pte.hu>
 * Copyright (C) 2012       Thunor <thunorsif@hotmail.com>
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
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* 
**
** $Id: stringman.c,v 1.1 2004/11/16 20:43:31 root Exp root $
** $Log: stringman.c,v $
** Revision 1.1  2004/11/16 20:43:31  root
** Initial revision
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "gtk3d.h"
#include "stringman.h"
#include "widgets.h"

#define IN_TEXT         1
#define IN_SEPARATOR    2

#ifdef G_OS_WIN32
ssize_t
getline(char **lineptr, size_t *n, FILE *stream)
{
	g_warning("%s(): Unimplemented", __func__);
	return 0;
}
#endif

gboolean
input_is_shell_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "command:", 8) == 0;
}

const gchar *
input_get_shell_command(const gchar *command)
{
	if (input_is_shell_command(command))
		return command + 8;
	else
		return NULL;
}

/* Redundant: Not being used
gboolean
command_is_shell_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "command:", 8) == 0;
}

const gchar *
command_get_shell_command(const gchar *command)
{
	if (command_is_shell_command(command))
		return command + 8;
	else
		return NULL;
}

gboolean
command_is_exit_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "exit:", 5) == 0;
}

const gchar *
command_get_exit_command(const gchar *command)
{
	if (command_is_exit_command(command))
		return command + 5;
	else
		return NULL;
}

gboolean
command_is_closewindow_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "closewindow:", 12) == 0;
}

const gchar *
command_get_closewindow_command(const gchar *command)
{
	if (command_is_closewindow_command(command))
		return command + 12;
	else
		return NULL;
}

gboolean
command_is_launch_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "launch:", 7) == 0;
}

const gchar *
command_get_launch_command(const gchar *command)
{
	if (command_is_launch_command(command))
		return command + 7;
	else
		return NULL;
}

gboolean
command_is_enable_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "enable:", 7) == 0;
}

const gchar *
command_get_enable_command(const gchar *command)
{
	if (command_is_enable_command(command))
		return command + 7;
	else
		return NULL;
}


gboolean
command_is_disable_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "disable:", 8) == 0;
}

const gchar *
command_get_disable_command(const gchar *command)
{
	if (command_is_disable_command(command))
		return command + 8;
	else
		return NULL;
}


gboolean
command_is_refresh_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "refresh:", 8) == 0;
}

const gchar *
command_get_refresh_command(const gchar *command)
{
	if (command_is_refresh_command(command))
		return command + 8;
	else
		return NULL;
}


gboolean
command_is_save_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "save:", 5) == 0;
}

const gchar *
command_get_save_command(const gchar *command)
{
	if (command_is_save_command(command))
		return command + 5;
	else
		return NULL;
}


gboolean
command_is_fileselect_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "fileselect:", 11) == 0;
}

const gchar *
command_get_fileselect_command(const gchar *command)
{
	if (command_is_fileselect_command(command))
		return command + 11;
	else
		return NULL;
}

gboolean
command_is_clear_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "clear:", 6) == 0;
}

const gchar *
command_get_clear_command(const gchar *command)
{
	if (command_is_clear_command(command))
		return command + 6;
	else
		return NULL;
}


gboolean
command_is_removeselected_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "removeselected:", 15) == 0;
}

const gchar *
command_get_removeselected_command(const gchar *command)
{
	if (command_is_removeselected_command(command))
		return command + 15;
	else
		return NULL;
}


gboolean
command_is_insert_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "insert:", 7) == 0;
}

const gchar *
command_get_insert_command(const gchar *command)
{
	if (command_is_insert_command(command))
		return command + 7;
	else
		return NULL;
}


gboolean
command_is_append_command(const gchar *command)
{
	if (command == NULL)
		return FALSE;
	return strncasecmp(command, "append:", 7) == 0;
}

const gchar *
command_get_append_command(const gchar *command)
{
	if (command_is_append_command(command))
		return command + 7;
	else
		return NULL;
} */


/* Thunor: These need to match CommandType as defined in stringman.h */

static gchar *prefixes[] = {
	"command",
	"exit",
	"closewindow",
	"launch",
	"enable",
	"disable",
	"show",
	"hide",
	"activate",
	"grabfocus",
	"presentwindow",
	"refresh",
	"save",
	"fileselect",
	"clear",
	"removeselected",
	"break",
	"insert",
	"append",
	"pulse",
	"start",
	"stop",
	NULL
};

CommandType
command_prefix_get_type(const gchar *prefix) 
{
	gint n;
	for (n = 0; prefixes[n] != NULL; ++n) 
		if (g_ascii_strcasecmp(prefix, prefixes[n]) == 0)
			return (CommandType) n;

	return CommandShellCommand;
}

void
command_get_prefix(const gchar *string, 
		gchar **prefix,
		gchar **command)
{
	gchar *separator;
	gchar *tmp;
	gint   n;

	g_assert(string != NULL && prefix != NULL && command != NULL);
	separator = g_utf8_strchr(string, -1, ':');
	if (separator == NULL) {
		*prefix = g_strdup("command");
		*command = g_strchug(g_strdup(string));
		return;
	}
	
	n = g_utf8_pointer_to_offset(string, separator);
	tmp = g_strndup(string, n);

	for (n = 0; prefixes[n] != NULL; ++n) 
		if (g_ascii_strcasecmp(tmp, prefixes[n]) == 0) {
			*prefix = tmp;
			*command = g_strchug(g_strdup(separator + 1));
			return;
		}
	
	/*
	** We found the separator, but the prefix is invalid, so it is a shell
	** command with a ':' in it.
	*/
	g_free(tmp);
	*prefix = g_strdup("command");
	*command = g_strchug(g_strdup(string));
}

/* Thunor: I saw what technosaurus was doing with GTKDIALOG_PIXMAP_PATH
 * at http://www.murga-linux.com/puppy/viewtopic.php?p=632867#632867
 * and thought it a very good platform independent idea to implement */

char *find_pixmap(char *filename)
{
	static gchar retval[512];
	gchar fullpath[512];
	gchar **folders;
	gchar *gtkdialog_pixmap_path;
	gint count;

	g_strlcpy(retval, "", sizeof(retval));	/* Reset the return value */

	if (access(filename, R_OK) == 0) {

		g_strlcpy(retval, filename, sizeof(retval));

	} else {

		/* If the GTKDIALOG_PIXMAP_PATH envvar exists then attempt to find
		 * the file in any one of a number of user defined locations */
		if ((gtkdialog_pixmap_path = getenv("GTKDIALOG_PIXMAP_PATH"))) {

			folders = g_strsplit(gtkdialog_pixmap_path, ":", 16);

#ifdef DEBUG
			fprintf(stderr, "%s(): gtkdialog_pixmap_path=\"%s\"\n",
				__func__, gtkdialog_pixmap_path);
			count = 0;
			while (folders[count]) {
				fprintf(stderr, "%s(): folders[%i]=\"%s\"\n", __func__,
					count, folders[count]);
				count++;
			}
#endif

			count = 0;
			while (folders[count]) {
				g_strlcpy(fullpath, folders[count], sizeof(fullpath));
				/* Guard: only strip trailing slash if fullpath is non-empty */
				if (fullpath[0] != '\0' && fullpath[strlen(fullpath) - 1] != '/')
					g_strlcat(fullpath, "/", sizeof(fullpath));
				g_strlcat(fullpath, filename, sizeof(fullpath));

#ifdef DEBUG
				fprintf(stderr, "%s(): Finding \"%s\"\n", __func__,
					fullpath);
#endif

				if (access(fullpath, R_OK) == 0) {
					g_strlcpy(retval, fullpath, sizeof(retval));
					break;
				}

				count++;
			}

			if (folders) g_strfreev(folders);
		}

	}

#ifdef DEBUG
	fprintf(stderr, "%s(): Returning \"%s\"\n", __func__, retval);
#endif

	return retval;
}


/* Decoupe str sur le separateur fs.
 *
 * ATTENTION, contrat d'appartenance contre-intuitif. Le commentaire d'origine
 * affirmait « does not destroy the original string, rather it makes duplicates
 * of it » : c'est faux sur les deux points.
 *
 *   1. str est MODIFIE SUR PLACE : chaque separateur devient un '\0'.
 *   2. line[0] N'EST PAS une copie, c'est str lui-meme. Seuls line[1] et
 *      les suivants sont dupliques.
 *
 * Comme list_t_free() libere line[0] avec g_free(), l'appelant CEDE la
 * propriete de str : il doit passer une chaine allouee par GLib, jamais un
 * litteral ni un tampon de pile. Tous les appels du projet passent donc un
 * g_strdup().
 */
list_t *linecutter(char *str, int fs)
{
	list_t *parts;
	int n, q;
	
	g_assert(str != NULL);
#ifdef DEBUG
	g_message("%s(%s, %c)", __func__, str, fs);
#endif

	parts = g_malloc(sizeof(list_t));
	parts->n_lines = 1;
	parts->maxlines = 128;
	parts->line = g_malloc(sizeof(char *) * 128);

	/* Thunor: These need to be nullified because some functions may
	 * require more parts than were found and they will expect NULL */
	for (n = 0; n < 128; n++) parts->line[n] = NULL;

	parts->line[0] = str;
	
	for(n = 0; n <= (gint)strlen(str); ++n){
		if (str[n] == fs){
			/* g_strdup et non strdup : list_t_free libere par g_free,
			 * et melanger les deux allocateurs est indefini. */
			parts->line[parts->n_lines] = g_strdup(str + n + 1);
			++parts->n_lines;
		}
	}
	
	parts->line[parts->n_lines] = NULL;

	for (n = 0; n < parts->n_lines - 1; ++n){
		for (q = 0; q < (gint)strlen(parts->line[n]); ++q)
			if (*(parts->line[n] + q) == fs)
				*(parts->line[n] + q) = '\0';
	}
	return parts;
}

/* Thunor: The linecutter function above is allocating memory for the
 * returned list_t structure but nowhere within the project have I seen
 * these structures being freed, so I've created a function to do it */

void list_t_free(list_t *ptr)
{
	gint count;

	if (ptr != NULL) {
		for (count = 0; count < ptr->n_lines; count++)
			g_free(ptr->line[count]);
		g_free(ptr->line);
		g_free(ptr);
		ptr = NULL;
	}
}

/*
** This function will give a default name for widgets, so every
** widget can get a variable name.
*/
char *str_default_name(int itype)
{
	static int serial = 0;
	char *type;
	char *name;

	name = g_malloc(64);

	++serial;
	type = widgets_to_str(itype);
	g_snprintf(name, 64, "%s%03d", type, serial);
	return (name);
}

/***********************************************************************
 * strnatcmp                                                           *
 ***********************************************************************/

gint strnatcmp(gchar *c1, gchar *c2, gint sensitive)
{
	gint              c1val;
	gint              c2val;
	gint              retval;

	if (c1 == c2) {		/* Deals with both NULL, both empty, both equal */
		retval = 0;
	} else if (!c1) {
		retval = -1;
	} else if (!c2) {
		retval = 1;
	} else if (!*c1) {
		retval = -1;
	} else if (!*c2) {
		retval = 1;
	} else {
		/* There's definitely something different to compare */
		while (TRUE) {

			/* Ignore whitespace */
			while ((*c1 == ' ' || *c1 == '\t') && *(c1 + 1)) c1++;
			while ((*c2 == ' ' || *c2 == '\t') && *(c2 + 1)) c2++;

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s(): c1='%s' c2='%s'\n", __func__, c1, c2);
#endif

			/* Evaluate data being pointed to */
			if ((*c1 >= '0' && *c1 <= '9')) {
				/* Evaluate consecutive numbers */
				c1val = 0;
				while (*c1 >= '0' && *c1 <= '9') {
					/* Note: All leading zeroes evaluate as 0 */
					/* This is Unicode safe */
					c1val = c1val * 10 + *c1 - '0';
					c1++;
				}
				c1val += 1000;	/* Add some weight */
			} else {
				/* Evaluate single chars */
				if (sensitive) {
					c1val = *c1;
				} else {
					/* tolower() determines what is upper
					 * and lower case dependent upon locale */
					c1val = tolower(*c1);
				}
				c1++;
			}
			if ((*c2 >= '0' && *c2 <= '9')) {
				/* Evaluate consecutive numbers */
				c2val = 0;
				while (*c2 >= '0' && *c2 <= '9') {
					/* Note: All leading zeroes evaluate as 0 */
					/* This is Unicode safe */
					c2val = c2val * 10 + *c2 - '0';
					c2++;
				}
				c2val += 1000;	/* Add some weight */
			} else {
				/* Evaluate single chars */
				if (sensitive) {
					c2val = *c2;
				} else {
					/* tolower() determines what is upper
					 * and lower case dependent upon locale */
					c2val = tolower(*c2);
				}
				c2++;
			}

			/* At this point both c1 and c2 will be pointing to the next
			 * character to process which could be the terminating zero */

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s(): c1val=%i c2val=%i c1='%s' c2='%s'\n", __func__, c1val, c2val, c1, c2);
#endif

			/* Not equal? */
			if (c1val < c2val) {
				retval = -1;
				break;
			} else if (c1val > c2val) {
				retval = 1;
				break;
			}

			/* End of data? */
			if (!*c1 && !*c2) {
				retval = 0;
				break;
			} else if (!*c1) {
				retval = -1;
				break;
			} else if (!*c2) {
				retval = 1;
				break;
			}
		}
	}

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s(): retval=%i\n", __func__, retval);
#endif

	return retval;
}
