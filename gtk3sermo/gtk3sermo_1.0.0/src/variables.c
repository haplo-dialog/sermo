#include "safe_exec.h"
/*
 * variables.c:
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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"
#include "gtk3d.h"
#include "variables.h"
#include "widgets.h"
#include "widget_button.h"
#include "widget_checkbox.h"
#include "widget_colorbutton.h"
#include "widget_combobox.h"
#include "widget_comboboxtext.h"
#include "widget_edit.h"
#include "widget_entry.h"
#include "widget_eventbox.h"
#include "widget_expander.h"
#include "widget_switch.h"
#include "widget_filechooser.h"
#include "widget_calendar.h"
#include "widget_linkbutton.h"
#include "widget_searchentry.h"
#include "widget_infobar.h"
#include "widget_spinner.h"
#include "widget_levelbar.h"
#include "widget_drawingarea.h"
#include "widget_image.h"
#include "widget_pulse.h"
#include "widget_password.h"
#include "widget_aspectframe.h"
#include "widget_vseparator.h"
#include "widget_vscale.h"
#include "widget_fontbutton.h"
#include "widget_frame.h"
#include "widget_hbox.h"
#include "widget_hscale.h"
#include "widget_hseparator.h"
#include "widget_list.h"
#include "widget_menubar.h"
#include "widget_menuitem.h"
#include "widget_notebook.h"
#include "widget_pixmap.h"
#include "widget_progressbar.h"
#include "widget_radiobutton.h"
#include "widget_spinbutton.h"
#include "widget_statusbar.h"
#include "widget_table.h"
#include "widget_terminal.h"
#include "widget_text.h"
#include "widget_timer.h"
#include "widget_tree.h"
#include "widget_vbox.h"
#include "widget_window.h"
#include "tag_attributes.h"
#if HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>
#endif

extern gboolean option_no_warning;

/* Local function prototypes */
#ifdef DEBUG
void variables_print_one(variable *var);
#endif
variable *variables_new(const char *name);
/* Redundant: Not being used: variable *variables_set_widget(const char *name, GtkWidget *widget); */
/* Redundant: Not being used: variable *variables_set_parent(const char *name, GtkWidget *parent); */
/* Redundant: Not being used: variable *variables_set_type(const char *name, int type); */
gboolean variables_is_avail_by_name(const char *name);
int _tree_insert(variable *new, variable *actual);
static variable *_tree_find(const char *name, variable *actual);
static gint do_variables_count_widgets(variable *actual, gint n);
static variable *do_find_variable_by_widget(variable *actual, GtkWidget *widget);
static void _variables_initialize(variable *actual);
static void _variables_export(variable * actual);
#ifdef DEBUG
void variables_print_debug(variable *actual);
#endif

variable *root = NULL;

/***********************************************************************
 * T13 fix: shell_escape_value()                                       *
 * Escape backslashes and double-quotes in a widget value so that the  *
 * shell output line  VARNAME="<value>"  can be safely eval'd.         *
 * Caller must g_free() the returned string.                           *
 ***********************************************************************/
gchar *
shell_escape_value(const gchar *value)
{
	GString *out;
	const gchar *p;

	if (value == NULL)
		return g_strdup("");

	out = g_string_sized_new(strlen(value) + 8);
	for (p = value; *p; p++) {
		if (*p == '\\' || *p == '"')
			g_string_append_c(out, '\\');
		g_string_append_c(out, *p);
	}
	return g_string_free(out, FALSE);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

#ifdef DEBUG
void variables_print_one(variable *var)
{
	fprintf(stderr, "Name: %s\n", var->Name);
	fprintf(stderr, "  Widget: %p\n", var->Widget);
	fprintf(stderr, "  Type: %s\n", widgets_to_str(var->Type));
	fprintf(stderr, "  Parent: %p\n", var->ParentWindow);
	fflush(stderr);
}
#endif

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function will create a new variable */

variable *variables_new(const char *name)
{
	variable *new;

#ifdef DEBUG
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* 
	 ** If the variable exists we simply returns without making a 
	 ** a warning. 
	 */
	if (variables_is_avail_by_name(name))
		return (variables_get_by_name(name));

	new = g_malloc(sizeof(variable));
	/* g_strlcpy always NUL-terminates and truncates cleanly, unlike strncpy
	 * on an over-long name into a non-zeroed buffer (CWE-125/CWE-170). */
	g_strlcpy(new->Name, name, sizeof(new->Name));
	/* 
	 ** Setting the defaults for this variable.
	 */
	new->Widget = NULL;
	new->window_id = 0;
	new->Attributes = NULL;
	new->Type = 0;
	new->row = -1;
	new->column = -1;
	new->ncolumns = -1;

	new->left = NULL;
	new->right = NULL;

	_tree_insert(new, NULL);

#ifdef DEBUG
	fprintf(stderr, "%s(): Name=%s\n", __func__, new->Name);
#endif

#ifdef DEBUG
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return new;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function will create a new variable with widget and type.
 * 
 * Thunor: The way in which this operates is flawed but it works:
 * Attempting to create a variable with a duplicate name results in the
 * original variable's widget pointer being overwritten and in fact all
 * of the original widget's attributes will be overwritten too. So if
 * you call find_variable_by_widget() you'll get NULL for the original
 * widget and in any case surely the application will be compromised?
 * 
 * Now, if you try to enforce unique variable names you'll encounter a
 * problem in that launched windows' widget's are destroyed but their
 * variables aren't, and if you launch the same window again then the
 * variables will already exist and an attempt will be made to recreate
 * them but because they already exist they'll be reused, therefore
 * enforcing unique variable names is not going to be possible.
 * 
 * Basically you have accept that the system has been designed to reuse
 * variables even though it could result in widgets with NULL variables
 * and a compromised application.
 * 
 * It's not even possible to detect duplicate variable names after the
 * initial window has been created so it's pointless making the effort
 * to warn the user.
 */

variable *variables_new_with_widget(AttributeSet *Attr,
	tag_attr *widget_tag_attr, GtkWidget *widget, int type)
{
	GList *element;
	char *name;
	variable *var;
	int autonamed = FALSE;

#ifdef DEBUG
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_assert(Attr != NULL);
	g_assert(widget != NULL);

	/*
	 ** If the widget have no name we create a default one for it.
	 */
	if (!attributeset_is_avail(Attr, ATTR_VARIABLE)) {
		attributeset_insert(Attr,
				    ATTR_VARIABLE,
				    (char *) str_default_name(type));
		autonamed = TRUE;
	}

	name = attributeset_get_first(&element, Attr, ATTR_VARIABLE);

	/*
	 * Retrieve or create the backing variable node.
	 *
	 * T9 fix — duplicate window names:
	 *   When the same dialog XML is opened a second time while the first
	 *   instance is still live, both instances share the same widget names.
	 *   The old code silently overwrote the first window's Widget pointer,
	 *   leaving it with a dangling reference and producing unpredictable
	 *   behaviour (crashes, wrong-widget refreshes, …).
	 *
	 *   Detection: variable already exists AND its Widget is non-NULL
	 *   (i.e. the owning window is still open) AND it belongs to a
	 *   different window_id than the one being constructed now.
	 *
	 *   Cure: register the new window's widget under a suffixed name
	 *   "<NAME>__W<window_id>" (or "<NAME>__W<window_id>_N" if that too
	 *   is already taken).  This keeps the first window's variable intact
	 *   and makes the collision visible in the shell output.
	 *
	 *   autonamed variables are exempt: each invocation of str_default_name()
	 *   produces a unique counter-based slot, so there is never a true
	 *   collision among them.
	 */
	if (!variables_is_avail_by_name(name)) {
		var = variables_new(name);
	} else {
		var = variables_get_by_name(name);

		if (!autonamed && var->Widget != NULL &&
		    var->window_id != window_id) {
			/* Build a unique suffixed name for this window's copy. */
			gchar unique[NAMELEN + 1];
			int   n = 2;

			g_snprintf(unique, sizeof(unique),
				   "%s__W%d", name, window_id);
			while (variables_is_avail_by_name(unique))
				g_snprintf(unique, sizeof(unique),
					   "%s__W%d_%d", name, window_id, n++);

			g_warning("%s(): variable \"%s\" is already owned by "
				  "window_id=%d (Widget=%p); registering duplicate "
				  "as \"%s\" for window_id=%d to avoid silent clobber.",
				  __func__, name,
				  var->window_id, (void *)var->Widget,
				  unique, window_id);

			var = variables_new(unique);
		}
	}

	g_assert(var != NULL);

	var->Widget = widget;
	var->window_id = window_id;
	var->Type = type;
	var->Attributes = Attr;
	var->widget_tag_attr = widget_tag_attr;
	var->autonamed = autonamed;

#ifdef DEBUG
	fprintf(stderr, "%s(): Name=%s Widget=%p window_id=%i Type=%i\n",
		__func__, var->Name, var->Widget, var->window_id, var->Type);
#endif

#ifdef DEBUG
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return (var);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

/* Redundant: Not being used.
variable *variables_set_widget(const char *name, GtkWidget *widget)
{
	variable *var;
#ifdef DEBUG
	g_message("%s(): variable '%s'.", __func__, name);
#endif
	var = variables_get_by_name(name);
	if (var == NULL)
		var = variables_new(name);

	var->Widget = widget;
	return (var);
} */

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

/* Redundant: Not being used.
#if 0
variable *variables_set_parent(const char *name, GtkWidget *parent)
{
	variable *var;
#ifdef DEBUG
	g_message("%s(): Start with '%s'", __func__, name);
#endif
	var = variables_get_by_name(name);
	if (var == NULL)
		var = variables_new(name);

	var->ParentWindow = parent;
#ifdef DEBUG
	g_message("%s(): End with '%s'", __func__, name);
#endif
	return var;
}
#endif */

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

/* Redundant: Not being used.
variable *variables_set_type(const char *name, int type)
{
	variable *var;
#ifdef DEBUG
	fprintf(stderr, "%s(): variable '%s'.\n", __func__, name);
	fflush(stderr);
#endif
	var = variables_get_by_name(name);
	if (var == NULL)
		var = variables_new(name);

	var->Type = type;
	return (var);
} */

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

variable *variables_set_attributes(const char *name, AttributeSet *a)
{
	variable *var;
#ifdef DEBUG
	g_message("%s(): start '%s'", __func__, name);
#endif
	var = variables_get_by_name(name);
	if (var == NULL)
		var = variables_new(name);

	var->Attributes = a;
	return (var);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

variable *variables_set_row_column(const char *name, int row, int column)
{
	variable *toset;
#ifdef DEBUG
	fprintf(stderr, "%s(): variable: %s\n", __func__, name);
	fflush(stderr);
#endif
	toset = _tree_find(name, NULL);
	if (toset == NULL)
		return (NULL);

	toset->row = row;
	toset->column = column;
	return (toset);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function will set the value of the given variable. This means
 * that it will set a widget's data or insert data to the widget as the
 * type of the widget makes clear */

variable *variables_set_value(const char *name, const char *value)
{
	variable         *toset;
	gchar            *string;

#ifdef DEBUG
	fprintf(stderr, "%s(): variable '%s'.\n", __func__, name);
	fflush(stderr);
#endif
	g_assert(name != NULL && value != NULL);
	
	toset = _tree_find(name, NULL);

	if (toset == NULL)
		return (NULL);

	/* If the custom attribute "block-function-signals" is true
	 * then block signals whilst performing this function */
	if (toset->widget_tag_attr &&
		((string = get_tag_attribute(toset->widget_tag_attr, "block-function-signals"))) &&
		((strcasecmp(string, "true") == 0) || (strcasecmp(string, "yes") == 0) ||
		(atoi(string) == 1))) {
		GTKD_FUNCTION_SIGNALS_BLOCK;
	}

	switch (toset->Type) {
		case WIDGET_CANCELBUTTON:
		case WIDGET_HELPBUTTON:
		case WIDGET_NOBUTTON:
		case WIDGET_OKBUTTON:
		case WIDGET_YESBUTTON:
		case WIDGET_TOGGLEBUTTON:
		case WIDGET_BUTTON:
			widget_button_fileselect(toset, name, value);
			break;
		case WIDGET_CHECKBOX:
			widget_checkbox_fileselect(toset, name, value);
			break;
		case WIDGET_COLORBUTTON:
			widget_colorbutton_fileselect(toset, name, value);
			break;
		case WIDGET_COMBOBOX:
			widget_combobox_fileselect(toset, name, value);
			break;
		case WIDGET_COMBOBOXENTRY:
		case WIDGET_COMBOBOXTEXT:
			widget_comboboxtext_fileselect(toset, name, value);
			break;
		case WIDGET_EDIT:
			widget_edit_fileselect(toset, name, value);
			break;
		case WIDGET_ENTRY:
			widget_entry_fileselect(toset, name, value);
			break;
		case WIDGET_EVENTBOX:
			widget_eventbox_fileselect(toset, name, value);
			break;
		case WIDGET_EXPANDER:
			widget_expander_fileselect(toset, name, value);
			break;
		case WIDGET_SWITCH:
			widget_switch_fileselect(toset, name, value);
			break;
		case WIDGET_FILECHOOSER:
			widget_filechooser_fileselect(toset, name, value);
			break;
		case WIDGET_CALENDAR:
			widget_calendar_fileselect(toset, name, value);
			break;
		case WIDGET_LINKBUTTON:
			widget_linkbutton_fileselect(toset, name, value);
			break;
		case WIDGET_SEARCHENTRY:
			widget_searchentry_fileselect(toset, name, value);
			break;
		case WIDGET_INFOBAR:
			widget_infobar_fileselect(toset, name, value);
			break;
		case WIDGET_SPINNER:
			widget_spinner_fileselect(toset, name, value);
			break;
		case WIDGET_LEVELBAR:
			widget_levelbar_fileselect(toset, name, value);
			break;
		case WIDGET_DRAWINGAREA:
			widget_drawingarea_fileselect(toset, name, value);
			break;
		case WIDGET_IMAGE:
			widget_image_fileselect(toset, name, value);
			break;
		case WIDGET_PULSE:
			widget_pulse_fileselect(toset, name, value);
			break;
		case WIDGET_PASSWORD:
			widget_password_fileselect(toset, name, value);
			break;
		case WIDGET_ASPECTFRAME:
			widget_aspectframe_fileselect(toset, name, value);
			break;
		case WIDGET_FONTBUTTON:
			widget_fontbutton_fileselect(toset, name, value);
			break;
		case WIDGET_FRAME:
			widget_frame_fileselect(toset, name, value);
			break;
		case WIDGET_HBOX:
			widget_hbox_fileselect(toset, name, value);
			break;
		case WIDGET_HSCALE:
		case WIDGET_VSCALE:
			widget_hscale_fileselect(toset, name, value);
			break;
		case WIDGET_HSEPARATOR:
		case WIDGET_VSEPARATOR:
			widget_hseparator_fileselect(toset, name, value);
			break;
		case WIDGET_LIST:
			widget_list_fileselect(toset, name, value);
			break;
		case WIDGET_MENUBAR:
			widget_menubar_fileselect(toset, name, value);
			break;
		case WIDGET_MENUITEMSEPARATOR:
		case WIDGET_MENUITEM:
		case WIDGET_MENU:
			widget_menuitem_fileselect(toset, name, value);
			break;
		case WIDGET_NOTEBOOK:
			widget_notebook_fileselect(toset, name, value);
			break;
		case WIDGET_PIXMAP:
			widget_pixmap_fileselect(toset, name, value);
			break;
		case WIDGET_PROGRESSBAR:
			widget_progressbar_fileselect(toset, name, value);
			break;
		case WIDGET_RADIOBUTTON:
			widget_radiobutton_fileselect(toset, name, value);
			break;
		case WIDGET_SPINBUTTON:
			widget_spinbutton_fileselect(toset, name, value);
			break;
		case WIDGET_STATUSBAR:
			widget_statusbar_fileselect(toset, name, value);
			break;
		case WIDGET_TABLE:
			widget_table_fileselect(toset, name, value);
			break;
		case WIDGET_TERMINAL:
			widget_terminal_fileselect(toset, name, value);
			break;
		case WIDGET_TEXT:
			widget_text_fileselect(toset, name, value);
			break;
		case WIDGET_TIMER:
			widget_timer_fileselect(toset, name, value);
			break;
/* GTK3: always available - removed version guard */
		case WIDGET_TREE:
			widget_tree_fileselect(toset, name, value);
			break;
		case WIDGET_VBOX:
			widget_vbox_fileselect(toset, name, value);
			break;
		case WIDGET_WINDOW:
			widget_window_fileselect(toset, name, value);
			break;
		default:
			g_warning("%s(): Set-value not implemented for widget type %d.", __func__, toset->Type);
	}

	GTKD_FUNCTION_SIGNALS_RESET;

	return (toset);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

variable *variables_save(const char *name)
{
	variable *var;
	var = _tree_find(name, NULL);
	if (var == NULL)
		return (NULL);

	if (var->Widget == NULL)
		return (NULL);

	switch (var->Type) {
		case WIDGET_CANCELBUTTON:
		case WIDGET_HELPBUTTON:
		case WIDGET_NOBUTTON:
		case WIDGET_OKBUTTON:
		case WIDGET_YESBUTTON:
		case WIDGET_TOGGLEBUTTON:
		case WIDGET_BUTTON:
			widget_button_save(var);
			break;
		case WIDGET_CHECKBOX:
			widget_checkbox_save(var);
			break;
		case WIDGET_COLORBUTTON:
			widget_colorbutton_save(var);
			break;
		case WIDGET_COMBOBOX:
			widget_combobox_save(var);
			break;
		case WIDGET_COMBOBOXENTRY:
		case WIDGET_COMBOBOXTEXT:
			widget_comboboxtext_save(var);
			break;
		case WIDGET_EDIT:
			widget_edit_save(var);
			break;
		case WIDGET_ENTRY:
			widget_entry_save(var);
			break;
		case WIDGET_EVENTBOX:
			widget_eventbox_save(var);
			break;
		case WIDGET_EXPANDER:
			widget_expander_save(var);
			break;
		case WIDGET_SWITCH:
			widget_switch_save(var);
			break;
		case WIDGET_FILECHOOSER:
			widget_filechooser_save(var);
			break;
		case WIDGET_CALENDAR:
			widget_calendar_save(var);
			break;
		case WIDGET_LINKBUTTON:
			widget_linkbutton_save(var);
			break;
		case WIDGET_SEARCHENTRY:
			widget_searchentry_save(var);
			break;
		case WIDGET_INFOBAR:
			widget_infobar_save(var);
			break;
		case WIDGET_SPINNER:
			widget_spinner_save(var);
			break;
		case WIDGET_LEVELBAR:
			widget_levelbar_save(var);
			break;
		case WIDGET_DRAWINGAREA:
			widget_drawingarea_save(var);
			break;
		case WIDGET_IMAGE:
			widget_image_save(var);
			break;
		case WIDGET_PULSE:
			widget_pulse_save(var);
			break;
		case WIDGET_PASSWORD:
			widget_password_save(var);
			break;
		case WIDGET_ASPECTFRAME:
			widget_aspectframe_save(var);
			break;
		case WIDGET_FONTBUTTON:
			widget_fontbutton_save(var);
			break;
		case WIDGET_FRAME:
			widget_frame_save(var);
			break;
		case WIDGET_HBOX:
			widget_hbox_save(var);
			break;
		case WIDGET_HSCALE:
		case WIDGET_VSCALE:
			widget_hscale_save(var);
			break;
		case WIDGET_HSEPARATOR:
		case WIDGET_VSEPARATOR:
			widget_hseparator_save(var);
			break;
		case WIDGET_LIST:
			widget_list_save(var);
			break;
		case WIDGET_MENUBAR:
			widget_menubar_save(var);
			break;
		case WIDGET_MENUITEMSEPARATOR:
		case WIDGET_MENUITEM:
		case WIDGET_MENU:
			widget_menuitem_save(var);
			break;
		case WIDGET_NOTEBOOK:
			widget_notebook_save(var);
			break;
		case WIDGET_PIXMAP:
			widget_pixmap_save(var);
			break;
		case WIDGET_PROGRESSBAR:
			widget_progressbar_save(var);
			break;
		case WIDGET_RADIOBUTTON:
			widget_radiobutton_save(var);
			break;
		case WIDGET_SPINBUTTON:
			widget_spinbutton_save(var);
			break;
		case WIDGET_STATUSBAR:
			widget_statusbar_save(var);
			break;
		case WIDGET_TABLE:
			widget_table_save(var);
			break;
		case WIDGET_TERMINAL:
			widget_terminal_save(var);
			break;
		case WIDGET_TEXT:
			widget_text_save(var);
			break;
		case WIDGET_TIMER:
			widget_timer_save(var);
			break;
/* GTK3: always available - removed version guard */
		case WIDGET_TREE:
			widget_tree_save(var);
			break;
		case WIDGET_VBOX:
			widget_vbox_save(var);
			break;
		case WIDGET_WINDOW:
			widget_window_save(var);
			break;
		default:
			g_warning("%s(): Save not implemented for this widget.", __func__);
	}
	return (var);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function will actualize the variable by reinitializing widget */

variable *variables_refresh(const char *name)
{
	variable         *var;
	gchar            *string;
	gint              initialised = FALSE;

	g_assert(name != NULL);

#ifdef DEBUG
	g_message("%s(%s)", __func__, name);
#endif

	var = _tree_find(name, NULL);

	if (var == NULL || var->Widget == NULL)
		return NULL;

	g_assert(var->Attributes != NULL);

	/* T11 fix — widget init order:
	 * Export the current values of ALL widgets as environment variables
	 * before running this widget's <input> command.  Without this, an
	 * <input> command that references a sibling widget's variable finds
	 * the environment empty (or stale) during the initial show pass,
	 * causing the widget to display wrong data.
	 *
	 * This mirrors what action_shellcommand() and CommandRefresh already
	 * do before invoking user commands (see actions.c). */
	variables_export_all();

	/* Get initialised state of widget */
	if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
		initialised = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

	/* If the custom attribute "block-function-signals" is true
	 * then block signals whilst performing this function */
	if (var->widget_tag_attr &&
		((string = get_tag_attribute(var->widget_tag_attr, "block-function-signals"))) &&
		((strcasecmp(string, "true") == 0) || (strcasecmp(string, "yes") == 0) ||
		(atoi(string) == 1))) {
		GTKD_FUNCTION_SIGNALS_BLOCK;
	}

	switch (var->Type) {
		case WIDGET_CANCELBUTTON:
		case WIDGET_HELPBUTTON:
		case WIDGET_NOBUTTON:
		case WIDGET_OKBUTTON:
		case WIDGET_YESBUTTON:
		case WIDGET_TOGGLEBUTTON:
		case WIDGET_BUTTON:
			widget_button_refresh(var);
			break;
		case WIDGET_CHECKBOX:
			widget_checkbox_refresh(var);
			break;
		case WIDGET_COLORBUTTON:
			widget_colorbutton_refresh(var);
			break;
		case WIDGET_COMBOBOX:
			widget_combobox_refresh(var);
			break;
		case WIDGET_COMBOBOXENTRY:
		case WIDGET_COMBOBOXTEXT:
			widget_comboboxtext_refresh(var);
			break;
		case WIDGET_EDIT:
			widget_edit_refresh(var);
			break;
		case WIDGET_ENTRY:
			widget_entry_refresh(var);
			break;
		case WIDGET_EVENTBOX:
			widget_eventbox_refresh(var);
			break;
		case WIDGET_EXPANDER:
			widget_expander_refresh(var);
			break;
		case WIDGET_SWITCH:
			widget_switch_refresh(var);
			break;
		case WIDGET_FILECHOOSER:
			widget_filechooser_refresh(var);
			break;
		case WIDGET_CALENDAR:
			widget_calendar_refresh(var);
			break;
		case WIDGET_LINKBUTTON:
			widget_linkbutton_refresh(var);
			break;
		case WIDGET_SEARCHENTRY:
			widget_searchentry_refresh(var);
			break;
		case WIDGET_INFOBAR:
			widget_infobar_refresh(var);
			break;
		case WIDGET_SPINNER:
			widget_spinner_refresh(var);
			break;
		case WIDGET_LEVELBAR:
			widget_levelbar_refresh(var);
			break;
		case WIDGET_DRAWINGAREA:
			widget_drawingarea_refresh(var);
			break;
		case WIDGET_IMAGE:
			widget_image_refresh(var);
			break;
		case WIDGET_PULSE:
			widget_pulse_refresh(var);
			break;
		case WIDGET_PASSWORD:
			widget_password_refresh(var);
			break;
		case WIDGET_ASPECTFRAME:
			widget_aspectframe_refresh(var);
			break;
		case WIDGET_FONTBUTTON:
			widget_fontbutton_refresh(var);
			break;
		case WIDGET_FRAME:
			widget_frame_refresh(var);
			break;
		case WIDGET_HBOX:
			widget_hbox_refresh(var);
			break;
		case WIDGET_HSCALE:
		case WIDGET_VSCALE:
			widget_hscale_refresh(var);
			break;
		case WIDGET_HSEPARATOR:
		case WIDGET_VSEPARATOR:
			widget_hseparator_refresh(var);
			break;
		case WIDGET_LIST:
			widget_list_refresh(var);
			break;
		case WIDGET_MENUBAR:
			widget_menubar_refresh(var);
			break;
		case WIDGET_MENUITEMSEPARATOR:
		case WIDGET_MENUITEM:
		case WIDGET_MENU:
			widget_menuitem_refresh(var);
			break;
		case WIDGET_NOTEBOOK:
			widget_notebook_refresh(var);
			break;
		case WIDGET_PIXMAP:
			widget_pixmap_refresh(var);
			break;
		case WIDGET_PROGRESSBAR:
			widget_progressbar_refresh(var);
			break;
		case WIDGET_RADIOBUTTON:
			widget_radiobutton_refresh(var);
			break;
		case WIDGET_SPINBUTTON:
			widget_spinbutton_refresh(var);
			break;
		case WIDGET_STATUSBAR:
			widget_statusbar_refresh(var);
			break;
		case WIDGET_TABLE:
			widget_table_refresh(var);
			break;
		case WIDGET_TERMINAL:
			widget_terminal_refresh(var);
			break;
		case WIDGET_TEXT:
			widget_text_refresh(var);
			break;
		case WIDGET_TIMER:
			widget_timer_refresh(var);
			break;
/* GTK3: always available - removed version guard */
		case WIDGET_TREE:
			widget_tree_refresh(var);
			break;
		case WIDGET_VBOX:
			widget_vbox_refresh(var);
			break;
		case WIDGET_WINDOW:
			widget_window_refresh(var);
			break;
		default:
			if (initialised)
				g_warning("%s(): Refresh not implemented for this widget.", __func__);
			break;
	}

	if (!initialised) {
		/* Store "initialised" as a piece of widget data to record that
		 * this widget has been through this function at start-up */
		g_object_set_data(G_OBJECT(var->Widget), "_initialised", (gpointer)1);
	}

	GTKD_FUNCTION_SIGNALS_RESET;

#ifdef DEBUG
	g_message("%s(): end", __func__);
#endif

	return var;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

variable *variables_enable(const char *name)
{
	variable *var;
#ifdef DEBUG
	fprintf(stderr, "%s(): %s\n", __func__, name);
	fflush(stderr);
#endif
	var = _tree_find(name, NULL);
	if (var == NULL)
		return (NULL);
	if (var->Widget == NULL)
		return (NULL);

	gtk_widget_set_sensitive(var->Widget, TRUE);
	return (var);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

variable *variables_disable(const char *name)
{
	variable *var;
#ifdef DEBUG
	fprintf(stderr, "%s(): %s\n", __func__, name);
	fflush(stderr);
#endif
	var = _tree_find(name, NULL);
	if (var == NULL)
		return (NULL);
	if (var->Widget == NULL)
		return (NULL);

	gtk_widget_set_sensitive(var->Widget, FALSE);
	return (var);
}

/***********************************************************************
 * Variables pulse — advance a GtkProgressBar (pulse mode) one step   *
 ***********************************************************************/

variable *variables_pulse(const char *name)
{
	variable *var;
	var = _tree_find(name, NULL);
	if (var == NULL)
		return NULL;
	if (var->Widget == NULL)
		return NULL;
	if (GTK_IS_PROGRESS_BAR(var->Widget))
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(var->Widget));
	return var;
}

/***********************************************************************
 * Variables spinner start — start a GtkSpinner animation             *
 ***********************************************************************/

variable *variables_spinner_start(const char *name)
{
	variable *var;
	var = _tree_find(name, NULL);
	if (var == NULL)
		return NULL;
	if (var->Widget == NULL)
		return NULL;
	if (GTK_IS_SPINNER(var->Widget))
		gtk_spinner_start(GTK_SPINNER(var->Widget));
	return var;
}

/***********************************************************************
 * Variables spinner stop — stop a GtkSpinner animation               *
 ***********************************************************************/

variable *variables_spinner_stop(const char *name)
{
	variable *var;
	var = _tree_find(name, NULL);
	if (var == NULL)
		return NULL;
	if (var->Widget == NULL)
		return NULL;
	if (GTK_IS_SPINNER(var->Widget))
		gtk_spinner_stop(GTK_SPINNER(var->Widget));
	return var;
}

/***********************************************************************
 * Variables show                                                      *
 ***********************************************************************/

variable *variables_show(const char *name)
{
	GtkWidget        *grandparent = NULL;
	GtkWidget        *parent = NULL;
	variable         *var;

#ifdef DEBUG
	fprintf(stderr, "%s(): %s\n", __func__, name);
	fflush(stderr);
#endif

	var = _tree_find(name, NULL);
	if (var == NULL)
		return (NULL);
	if (var->Widget == NULL)
		return (NULL);

	/* The widget could be inside a scrolled window or inside a viewport
	 * inside a scrolled window so we need to show all of the widgets */
	parent = gtk_widget_get_parent(var->Widget);
	if (parent) grandparent = gtk_widget_get_parent(parent);

	if (!(parent && ((GTK_IS_SCROLLED_WINDOW(parent)) ||
		(GTK_IS_VIEWPORT(parent))))) parent = NULL;

	if (!(parent && (GTK_IS_VIEWPORT(parent)) && grandparent &&
		GTK_IS_SCROLLED_WINDOW(grandparent))) grandparent = NULL;

	if (grandparent)
		gtk_widget_show(grandparent);

	if (parent)
		gtk_widget_show(parent);

	if (var->Type == WIDGET_BUTTON) {
		/* Button widgets can have up to four added child widgets */
		gtk_widget_show_all(var->Widget);
	} else if (var->Type == WIDGET_FRAME) {
		/* Frame widgets have an added child widget */
		gtk_widget_show(gtk_bin_get_child(GTK_BIN(var->Widget)));
		gtk_widget_show(var->Widget);
	} else {
		gtk_widget_show(var->Widget);
	}

	return (var);
}

/***********************************************************************
 * Variables hide                                                      *
 ***********************************************************************/

variable *variables_hide(const char *name)
{
	GtkWidget        *grandparent = NULL;
	GtkWidget        *parent = NULL;
	variable         *var;

#ifdef DEBUG
	fprintf(stderr, "%s(): %s\n", __func__, name);
	fflush(stderr);
#endif

	var = _tree_find(name, NULL);
	if (var == NULL)
		return (NULL);
	if (var->Widget == NULL)
		return (NULL);

	/* The widget could be inside a scrolled window or inside a viewport
	 * inside a scrolled window so we need to hide all of the widgets */
	parent = gtk_widget_get_parent(var->Widget);
	if (parent) grandparent = gtk_widget_get_parent(parent);

	if (!(parent && ((GTK_IS_SCROLLED_WINDOW(parent)) ||
		(GTK_IS_VIEWPORT(parent))))) parent = NULL;

	if (!(parent && (GTK_IS_VIEWPORT(parent)) && grandparent &&
		GTK_IS_SCROLLED_WINDOW(grandparent))) grandparent = NULL;

#ifdef DEBUG
	fprintf(stderr, "%s(): Hiding widget\n", __func__);
#endif
	gtk_widget_hide(var->Widget);

	if (parent) {
#ifdef DEBUG
		fprintf(stderr, "%s(): Hiding parent\n", __func__);
#endif
		gtk_widget_hide(parent);
	}

	if (grandparent) {
#ifdef DEBUG
		fprintf(stderr, "%s(): Hiding grandparent\n", __func__);
#endif
		gtk_widget_hide(grandparent);
	}

/* Redundant
	parent = gtk_widget_get_parent(var->Widget);
	if (parent != NULL) grandparent = gtk_widget_get_parent(parent);

	if (parent != NULL && GTK_IS_SCROLLED_WINDOW(parent)) {
		gtk_widget_hide(parent);
	} else if (parent != NULL && GTK_IS_VIEWPORT(parent)) {
		/$ A viewport will always be inside a scrolled window $/
		if (grandparent != NULL && GTK_IS_SCROLLED_WINDOW(grandparent)) {
			gtk_widget_hide(grandparent);
		}
	} else {
		gtk_widget_hide(var->Widget);
	}*/

	return (var);
}

/***********************************************************************
 * Variables activate                                                  *
 ***********************************************************************/

variable *variables_activate(const char *name)
{
	gboolean  retval;
	variable *var;

#ifdef DEBUG
	fprintf(stderr, "%s(): %s\n", __func__, name);
	fflush(stderr);
#endif

	var = _tree_find(name, NULL);
	if (var == NULL)
		return (NULL);
	if (var->Widget == NULL)
		return (NULL);

	retval = gtk_widget_activate(var->Widget);

	if (!retval)
		g_debug( "%s(): %s is not an activatable widget.", __func__, name);

	return (var);
}

/***********************************************************************
 * Variables grabfocus                                                 *
 ***********************************************************************/

variable *variables_grabfocus(const char *name)
{
	variable *var;

#ifdef DEBUG
	fprintf(stderr, "%s(): %s\n", __func__, name);
	fflush(stderr);
#endif

	var = _tree_find(name, NULL);
	if (var == NULL)
		return (NULL);
	if (var->Widget == NULL)
		return (NULL);

	gtk_widget_grab_focus(var->Widget);

	return (var);
}

/***********************************************************************
 * Variables presentwindow                                             *
 ***********************************************************************/

variable *variables_presentwindow(const char *name)
{
	variable *var;

#ifdef DEBUG
	fprintf(stderr, "%s(): %s\n", __func__, name);
	fflush(stderr);
#endif

	var = _tree_find(name, NULL);
	if (var == NULL)
		return (NULL);
	if (var->Widget == NULL)
		return (NULL);

	gtk_window_present(GTK_WINDOW(var->Widget));

	return (var);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

gboolean variables_is_avail_by_name(const char *name)
{
	if (_tree_find(name, NULL) == NULL)
		return (FALSE);
	else
		return (TRUE);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

variable *variables_get_by_name(const char *name)
{
	return _tree_find(name, NULL);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function inserts a new tree node. The node must have a unique
 * name or the function will abort the program */

int _tree_insert(variable *new, variable *actual)
{
	int compare;
	if (new == NULL) {
		g_debug( "%s(): Inserting a NULL element.", __func__);
		exit(EXIT_FAILURE);
	}

	if (root == NULL) {
		root = new;
		return (0);
	}

	if (actual == NULL)
		actual = root;

	compare = strncmp(new->Name, actual->Name, NAMELEN);

	if (compare == 0) {
		g_debug( "%s(): Inserting existing variable: '%s'.",
			__func__, new->Name);
		exit(EXIT_FAILURE);
	}

	if (compare < 0)
		if (actual->left == NULL) {
			actual->left = new;
			return (0);
		} else {
			return (_tree_insert(new, actual->left));
		}

	if (compare > 0)
		if (actual->right == NULL) {
			actual->right = new;
			return (0);
		} else {
			return (_tree_insert(new, actual->right));
		}

	return (0); /* unreachable: compare est toujours <0, ==0 ou >0 */
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function finds the variable by its name and returns the pointer
 * to it */

static variable *_tree_find(const char *name, variable *actual)
{
	int compare;

	if (actual == NULL)
		actual = root;
	if (actual == NULL)
		return NULL;

	compare = strcmp(name, actual->Name);

	if (compare == 0)
		return (actual);

	if (compare < 0)
		if (actual->left != NULL)
			return _tree_find(name, actual->left);
		else
			return NULL;

	if (compare > 0)
		if (actual->right != NULL)
			return _tree_find(name, actual->right);
		else
			return NULL;

	return NULL; /* unreachable */
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

static gint do_variables_count_widgets(variable *actual, gint n)
{
	if (actual == NULL)
		actual = root;

	if (actual == NULL)
		return n;

	if (actual->left != NULL)
		n = do_variables_count_widgets(actual->left, n);

	if (actual->Widget != NULL) 
		++n;

	if (actual->right != NULL)
		n = do_variables_count_widgets(actual->right, n);

	return n;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

gint variables_count_widgets(void) 
{
	return do_variables_count_widgets(NULL, 0);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function will drop all the widgets with the given parent */

void variables_drop_by_window_id(variable *actual, gint window_id)
{
	gint              index = 0;
#if HAVE_SYS_INOTIFY_H
	gchar             fdname[16];
	gchar             wdname[16];
	gint              fd, wd;
#else
	GFileMonitor     *monitor;
	gchar             name[16];
#endif

#ifdef DEBUG
	GtkWidget *ancestor;
#endif

#ifdef DEBUG
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (actual == NULL) actual = root;

	if (actual != NULL) {

		if (actual->left != NULL)
			variables_drop_by_window_id(actual->left, window_id);

		if (actual->Widget != NULL) {

#ifdef DEBUG
			fprintf(stderr, "%s(): Name=%s Widget=%p window_id=%i Type=%i\n",
				__func__, actual->Name, actual->Widget, actual->window_id, actual->Type);
#endif

			g_assert(GTK_IS_WIDGET(actual->Widget));

#ifdef DEBUG
			ancestor = gtk_widget_get_parent(actual->Widget);
			fprintf(stderr, "%s(): ancestor1=%p\n", __func__, ancestor);
			if (ancestor) ancestor = gtk_widget_get_parent(ancestor);
			fprintf(stderr, "%s(): ancestor2=%p\n", __func__, ancestor);
			if (ancestor) ancestor = gtk_widget_get_parent(ancestor);
			fprintf(stderr, "%s(): ancestor3=%p\n", __func__, ancestor);
			if (ancestor) ancestor = gtk_widget_get_parent(ancestor);
			fprintf(stderr, "%s(): ancestor4=%p\n", __func__, ancestor);
#endif

			//Redundant: if (gtk_widget_get_toplevel(actual->Widget) == Parent) {
			//Redundant: if (gtk_widget_get_ancestor(actual->Widget, GTK_TYPE_WINDOW) == Parent) {
			if (actual->window_id == window_id) {

				/* Timer callbacks cancel themselves when they
				 * detect that var and var->widget are NULL */

				/* Cancel any existing file monitors */
#if HAVE_SYS_INOTIFY_H
				while (TRUE) {
					g_snprintf(fdname, sizeof(fdname), "_inotifyfd%i", index);
					g_snprintf(wdname, sizeof(wdname), "_inotifywd%i", index);
					if ((g_object_get_data(G_OBJECT(actual->Widget), fdname)) &&
						(g_object_get_data(G_OBJECT(actual->Widget), wdname))) {
						fd = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(actual->Widget),
							fdname));
						wd = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(actual->Widget),
							wdname));
#ifdef DEBUG
						fprintf(stderr, "%s(): fd=%i wd=%i\n", __func__,
							fd, wd);
#endif
						inotify_rm_watch(fd, wd);
					} else {
						break;
					}
					index++;
				}
#else
				while (TRUE) {
					g_snprintf(name, sizeof(name), "_monitor%i", index);
					monitor = g_object_get_data(G_OBJECT(actual->Widget), name);
					if (monitor) {
#ifdef DEBUG
						fprintf(stderr, "%s(): cancelling %s\n", __func__, name);
#endif
						g_file_monitor_cancel(monitor);
						g_object_unref(monitor);
						/* I don't have access to file to unref it */
					} else {
						break;
					}
					index++;
				}
#endif

				actual->Widget = NULL;

#ifdef DEBUG
				fprintf(stderr, "%s(): Name=%s Widget=%p window_id=%i Type=%i\n",
					__func__, actual->Name, actual->Widget, actual->window_id, actual->Type);
#endif

			}
		}

		if (actual->right != NULL)
			variables_drop_by_window_id(actual->right, window_id);

	}

#ifdef DEBUG
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

static variable *do_find_variable_by_widget(variable *actual,
	GtkWidget *widget)
{
	variable *found;

	if (actual == NULL)
		return NULL;
	
	if (actual->left != NULL) {
		found = do_find_variable_by_widget(actual->left, widget);
		if (found != NULL)
			return found;
	}
	
	if (actual->Widget == widget)
		return actual;

	if (actual->right != NULL) {
		return do_find_variable_by_widget(actual->right, widget);
	}

	return NULL;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

variable *find_variable_by_widget(GtkWidget *widget)
{
	return do_find_variable_by_widget(root, widget);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

void variables_initialize_all(void)
{
	_variables_initialize(NULL);
}

/***********************************************************************
 * T11 fix — seed env with <default> values before widget_show_all()  *
 *                                                                     *
 * Walks the BST and, for every named widget that declares a <default> *
 * attribute, exports the default string into the process environment  *
 * with g_setenv(…, FALSE) so that it does NOT overwrite a value that  *
 * a preceding widget's refresh pass has already placed there.         *
 *                                                                     *
 * Called from run_program() (automaton.c) BEFORE widget_show_all().   *
 * This ensures that when the first wave of show-signal handlers fires  *
 * and each widget runs its <input> command, sibling widgets' defaults  *
 * are already visible in the child process environment.               *
 ***********************************************************************/

static void _variables_seed_defaults(variable *actual)
{
	GList       *element;
	const gchar *defval;

	if (actual == NULL)
		actual = root;
	if (actual == NULL)
		return;

	if (actual->left != NULL)
		_variables_seed_defaults(actual->left);

	if (actual->Widget != NULL && !actual->autonamed &&
	    actual->Attributes != NULL) {
		if (attributeset_is_avail(actual->Attributes, ATTR_DEFAULT)) {
			defval = attributeset_get_first(&element,
						       actual->Attributes,
						       ATTR_DEFAULT);
			if (defval != NULL)
				g_setenv(actual->Name, defval, FALSE);
		}
	}

	if (actual->right != NULL)
		_variables_seed_defaults(actual->right);
}

void variables_seed_defaults(void)
{
	_variables_seed_defaults(NULL);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
static void _variables_initialize(variable *actual)
{
	GList *element;
	char *socket_id;
	char command[128];
	int result;

#ifdef DEBUG
	fprintf(stderr, "%s: Start.\n", __func__);
	fflush(stderr);
#endif

	if (actual == NULL)
		actual = root;
	if (actual == NULL)
		return;

	if (actual->left != NULL)
		_variables_initialize(actual->left);

	if (actual->Widget != NULL && actual->Type == WIDGET_GVIM) {
		gtk_widget_show(actual->Widget);
		socket_id = attributeset_get_first(&element, actual->Attributes,
			ATTR_SOCKET);
		//printf("----->%p\n", actual->Widget->window); 
		//printf("----->%x\n", atoi(socket_id));        
		if (socket_id != NULL) {
			g_snprintf(command, sizeof(command), "gvim --socketid %s &",
				socket_id);
			result = safe_system(command);
		} else {
			g_warning("%s(): Socket ID is NULL\n", __func__);
		}
	}

	if (actual->right != NULL)
		_variables_initialize(actual->right);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function will export the variables as environment variables */

void variables_export_all(void)
{
	_variables_export(NULL);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

static void _variables_export(variable *actual)
{
	GList            *element;
	gchar            *act;
	gchar            *line;
	gchar            *value;
	gint              export = TRUE;

#ifdef DEBUG
	g_message("%s(%p)", __func__, actual);
#endif

	if (actual == NULL)
		actual = root;
	if (actual == NULL)
		return;

#ifdef DEBUG
	g_message("    actual->name = '%s'", actual->Name);
#endif

	if (actual->left != NULL)
		_variables_export(actual->left);

	/* Thunor: It's now possible to prevent the exporting of declared
	 * variables by using <variable export="false">VARNAME</variable> */
	if ((act = attributeset_get_first(&element, actual->Attributes,
		ATTR_VARIABLE)) &&
		(value = attributeset_get_this_tagattr(&element, actual->Attributes,
		ATTR_VARIABLE, "export")) &&
		((strcasecmp(value, "false") == 0) ||
		(strcasecmp(value, "no") == 0) ||
		(strcasecmp(value, "0") == 0))) {
		export = FALSE;
#ifdef DEBUG
		fprintf(stderr, "%s(): variable=%s export=%s\n",
			__func__, act, value);
#endif
	}

	/* Thunor: I've stopped this from exporting autonamed variables
	 * because it's pointless and will affect performance. There's no
	 * reason why this should export them when print_variables() doesn't
	 * so it's a bug -- the developer wouldn't even know they existed.
	if (actual->Widget != NULL) { */
	if (actual->Widget != NULL && !actual->autonamed && export) {
		//
		// To export only the active element
		//
		value = widget_get_text_value(actual->Widget, actual->Type);

		/* Dead code — GtkCList removed in GTK3; widget_table.c now uses
		 * GtkTreeView.  widget_get_text_value() handles WIDGET_TABLE correctly.
		if (actual->Type == WIDGET_TABLE && actual->row != -1) {
			gtk_clist_get_text(GTK_CLIST(actual->Widget),
					   actual->row, 0, &value);
		} */

		/* Export the active element value into the environment.
		 * g_setenv() handles memory safely (copies both name and value). */
		if (value != NULL)
			g_setenv(actual->Name, value, TRUE);

		/* Thunor: I've disabled this for performance reasons. Zigbert was
		 * experiencing terrible table performance which I've tested too.
		 * Initially it may have been practicable with simple dialogs to
		 * dump the entire contents of a widget on every signal, but it
		 * definitely isn't anymore.
		 * Note: the original #if 0 block was malformed (orphaned #endif and
		 * case labels outside a switch after preprocessing) — cleaned up. */
#if 0
		switch (actual->Type) {
			case WIDGET_LIST:
				line = g_strdup_printf("%s_ALL=\"", actual->Name);
				itemlist = gtk_container_get_children(GTK_CONTAINER(actual->Widget));
				n = 0;
				while (itemlist != NULL) {
					if (itemlist->data != NULL) {
						text = g_object_get_data(G_OBJECT(itemlist->data), "user_data");
						if (n == 0)
							tmp = g_strconcat(line, "'", text, "'", NULL);
						else
							tmp = g_strconcat(line, " '", text, "'", NULL);
						g_free(line);
						line = tmp;
						++n;
					}
					itemlist = itemlist->next;
				}
				g_setenv(actual->Name, line, TRUE);
				g_free(line);
				break;

			case WIDGET_TABLE:
				tmp = widget_table_envvar_all_construct(actual);
				g_setenv(actual->Name, tmp, TRUE);
				g_free(tmp);
				break;

			case WIDGET_TREE:
				tmp = widget_tree_envvar_all_construct(actual);
				g_setenv(actual->Name, tmp, TRUE);
				g_free(tmp);
				break;

			case WIDGET_COMBOBOXENTRY:
			case WIDGET_COMBOBOXTEXT:
				tmp = widget_comboboxtext_envvar_all_construct(actual);
				g_setenv(actual->Name, tmp, TRUE);
				g_free(tmp);
				break;
		}
#endif

	}

	if (actual->right != NULL)
		_variables_export(actual->right);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function is called when we want to send the variable's values
 * to the standard output */

void print_variables(variable *actual)
{
	GList            *element;
	gchar            *act;
	gchar            *value;
	gint              export = TRUE;

	if (actual == NULL)
		actual = root;

	if (actual == NULL)
		return;

	if (actual->left != NULL)
		print_variables(actual->left);

	/* Thunor: It's now possible to prevent the exporting of declared
	 * variables by using <variable export="false">VARNAME</variable> */
	if ((act = attributeset_get_first(&element, actual->Attributes,
		ATTR_VARIABLE)) &&
		(value = attributeset_get_this_tagattr(&element, actual->Attributes,
		ATTR_VARIABLE, "export")) &&
		((strcasecmp(value, "false") == 0) ||
		(strcasecmp(value, "no") == 0) ||
		(strcasecmp(value, "0") == 0))) {
		export = FALSE;
#ifdef DEBUG
		fprintf(stderr, "%s(): variable=%s export=%s\n",
			__func__, act, value);
#endif
	}

	if (actual->Widget != NULL && !actual->autonamed && export) {
		//
		// To print only the active element
		//
		value = widget_get_text_value(actual->Widget, actual->Type);

		/* Dead code — GtkCList removed in GTK3; widget_table.c now uses
		 * GtkTreeView.  widget_get_text_value() handles WIDGET_TABLE correctly.
		if (actual->Type == WIDGET_TABLE && actual->row != -1) {
			gtk_clist_get_text(GTK_CLIST(actual->Widget),
					   actual->row, 0, &value);
		} */

		if (value == NULL)
			value = "";
		{
			/* T13 fix: escape backslashes and double-quotes so that the
			 * shell line  VARNAME="<value>"  can be safely eval'd even
			 * when the widget value itself contains those characters. */
			gchar *escaped = shell_escape_value(value);
			printf("%s=\"%s\"\n", actual->Name, escaped);
			g_free(escaped);
		}

		/* Thunor: I've disabled this for performance reasons.
		 * Note: the original #if 0 block was malformed — cleaned up. */
#if 0
		switch (actual->Type) {
			case WIDGET_LIST:
				n = 0;
				printf("%s_ALL=\"", actual->Name);
				itemlist = gtk_container_get_children(GTK_CONTAINER(actual->Widget));
				while (itemlist != NULL) {
					if (itemlist->data != NULL) {
						if (n == 0)
							printf("'%s'", (char*)g_object_get_data(G_OBJECT(itemlist->data), "user_data"));
						else
							printf(" '%s'", (char*)g_object_get_data(G_OBJECT(itemlist->data), "user_data"));
						++n;
					}
					itemlist = itemlist->next;
				}
				printf("\"\n");
				break;

			case WIDGET_TABLE:
				tmp = widget_table_envvar_all_construct(actual);
				g_printf("%s", tmp);
				g_free(tmp);
				break;

			case WIDGET_TREE:
				tmp = widget_tree_envvar_all_construct(actual);
				g_printf("%s", tmp);
				g_free(tmp);
				break;

			case WIDGET_COMBOBOXENTRY:
			case WIDGET_COMBOBOXTEXT:
				tmp = widget_comboboxtext_envvar_all_construct(actual);
				g_printf("%s", tmp);
				g_free(tmp);
				break;
		}
#endif

	}

	if (actual->right != NULL)
		print_variables(actual->right);

	fflush(stdout);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

int append_fromto_variable(const char *from, const char *to)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	variable     *var_from, *var_to;
	char         *value;
	GtkWidget    *item;
	GList        *glist = NULL;

	g_assert(from != NULL);
	g_assert(to != NULL);

#ifdef DEBUG
	g_message("%s(): from: '%s' to: '%s'", __func__, from, to);
#endif

	var_from = _tree_find(from, NULL);
	var_to = _tree_find(to, NULL);
	if (var_from == NULL || var_to == NULL) {
		g_warning("%s(): Insert: variable not found.", __func__);
		return FALSE;
	}

	value = g_strdup(widget_get_text_value(var_from->Widget, var_from->Type));

	if (strlen(value) == 0)
		return FALSE;

	switch (var_to->Type) {
		case WIDGET_LIST:
			/* GTK3: gtk_list_item_new_with_label → GtkListBoxRow + GtkLabel */
			item = gtk_list_box_row_new();
			{
				GtkWidget *label = gtk_label_new(value);
				gtk_label_set_xalign(GTK_LABEL(label), 0.0);
				gtk_container_add(GTK_CONTAINER(item), label);
			}
			g_object_set_data(G_OBJECT(item), "user_data", (gpointer) value);
			/* gtk_list_prepend_items → prepend a row at index 0 */
			gtk_list_box_prepend(GTK_LIST_BOX(var_to->Widget), item);
			gtk_widget_show_all(item);
			/* Select first row */
			gtk_list_box_select_row(GTK_LIST_BOX(var_to->Widget),
				gtk_list_box_get_row_at_index(GTK_LIST_BOX(var_to->Widget), 0));
		break;
		
		case WIDGET_ENTRY:
			gtk_entry_set_text(GTK_ENTRY(var_to->Widget), value);
			break;
		
		case WIDGET_EDIT:
			gtk_text_buffer_insert_at_cursor(
					gtk_text_view_get_buffer(GTK_TEXT_VIEW(var_to->Widget)), 
					value, g_utf8_strlen(value, -1));
			break;
/* GTK3: always available - removed version guard */
		case WIDGET_TREE:
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(var_to->Widget));
			gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL); 
			gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 1, value, -1);
			break;

/* GTK3: always available - removed version guard */
		case WIDGET_CHOOSER:
			gtk_file_chooser_add_shortcut_folder(
					GTK_FILE_CHOOSER(var_to->Widget), 
					value, NULL);
		break;
	default:
		g_warning("%s(): Copy not implemented for this widget.", __func__);
		return FALSE;
	}
	return TRUE;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function will clear the variable by deleting everything from
 * the widget */

variable *variables_clear(const char *name)
{
	variable         *toclear;
	gchar            *string;

#ifdef DEBUG
	fprintf(stderr, "%s(): variable: %s\n", __func__, name);
	fflush(stderr);
#endif

	toclear = _tree_find(name, NULL);
	if (toclear == NULL)
		return (NULL);

	/* If the custom attribute "block-function-signals" is true
	 * then block signals whilst performing this function */
	if (toclear->widget_tag_attr &&
		((string = get_tag_attribute(toclear->widget_tag_attr, "block-function-signals"))) &&
		((strcasecmp(string, "true") == 0) || (strcasecmp(string, "yes") == 0) ||
		(atoi(string) == 1))) {
		GTKD_FUNCTION_SIGNALS_BLOCK;
	}

	switch (toclear->Type) {
		case WIDGET_CANCELBUTTON:
		case WIDGET_HELPBUTTON:
		case WIDGET_NOBUTTON:
		case WIDGET_OKBUTTON:
		case WIDGET_YESBUTTON:
		case WIDGET_TOGGLEBUTTON:
		case WIDGET_BUTTON:
			widget_button_clear(toclear);
			break;
		case WIDGET_CHECKBOX:
			widget_checkbox_clear(toclear);
			break;
		case WIDGET_COLORBUTTON:
			widget_colorbutton_clear(toclear);
			break;
		case WIDGET_COMBOBOX:
			widget_combobox_clear(toclear);
			break;
		case WIDGET_COMBOBOXENTRY:
		case WIDGET_COMBOBOXTEXT:
			widget_comboboxtext_clear(toclear);
			break;
		case WIDGET_EDIT:
			widget_edit_clear(toclear);
			break;
		case WIDGET_ENTRY:
			widget_entry_clear(toclear);
			break;
		case WIDGET_EVENTBOX:
			widget_eventbox_clear(toclear);
			break;
		case WIDGET_EXPANDER:
			widget_expander_clear(toclear);
			break;
		case WIDGET_SWITCH:
			widget_switch_clear(toclear);
			break;
		case WIDGET_FILECHOOSER:
			widget_filechooser_clear(toclear);
			break;
		case WIDGET_CALENDAR:
			widget_calendar_clear(toclear);
			break;
		case WIDGET_LINKBUTTON:
			widget_linkbutton_clear(toclear);
			break;
		case WIDGET_SEARCHENTRY:
			widget_searchentry_clear(toclear);
			break;
		case WIDGET_INFOBAR:
			widget_infobar_clear(toclear);
			break;
		case WIDGET_SPINNER:
			widget_spinner_clear(toclear);
			break;
		case WIDGET_LEVELBAR:
			widget_levelbar_clear(toclear);
			break;
		case WIDGET_DRAWINGAREA:
			widget_drawingarea_clear(toclear);
			break;
		case WIDGET_IMAGE:
			widget_image_clear(toclear);
			break;
		case WIDGET_PULSE:
			widget_pulse_clear(toclear);
			break;
		case WIDGET_PASSWORD:
			widget_password_clear(toclear);
			break;
		case WIDGET_ASPECTFRAME:
			widget_aspectframe_clear(toclear);
			break;
		case WIDGET_FONTBUTTON:
			widget_fontbutton_clear(toclear);
			break;
		case WIDGET_FRAME:
			widget_frame_clear(toclear);
			break;
		case WIDGET_HBOX:
			widget_hbox_clear(toclear);
			break;
		case WIDGET_HSCALE:
		case WIDGET_VSCALE:
			widget_hscale_clear(toclear);
			break;
		case WIDGET_HSEPARATOR:
		case WIDGET_VSEPARATOR:
			widget_hseparator_clear(toclear);
			break;
		case WIDGET_LIST:
			widget_list_clear(toclear);
			break;
		case WIDGET_MENUBAR:
			widget_menubar_clear(toclear);
			break;
		case WIDGET_MENUITEMSEPARATOR:
		case WIDGET_MENUITEM:
		case WIDGET_MENU:
			widget_menuitem_clear(toclear);
			break;
		case WIDGET_NOTEBOOK:
			widget_notebook_clear(toclear);
			break;
		case WIDGET_PIXMAP:
			widget_pixmap_clear(toclear);
			break;
		case WIDGET_PROGRESSBAR:
			widget_progressbar_clear(toclear);
			break;
		case WIDGET_RADIOBUTTON:
			widget_radiobutton_clear(toclear);
			break;
		case WIDGET_SPINBUTTON:
			widget_spinbutton_clear(toclear);
			break;
		case WIDGET_STATUSBAR:
			widget_statusbar_clear(toclear);
			break;
		case WIDGET_TABLE:
			widget_table_clear(toclear);
			break;
		case WIDGET_TERMINAL:
			widget_terminal_clear(toclear);
			break;
		case WIDGET_TEXT:
			widget_text_clear(toclear);
			break;
		case WIDGET_TIMER:
			widget_timer_clear(toclear);
			break;
/* GTK3: always available - removed version guard */
		case WIDGET_TREE:
			widget_tree_clear(toclear);
			break;
		case WIDGET_VBOX:
			widget_vbox_clear(toclear);
			break;
		case WIDGET_WINDOW:
			widget_window_clear(toclear);
			break;
		default:
			g_warning("%s(): Clear not implemented for this widget.", __func__);
	}

	GTKD_FUNCTION_SIGNALS_RESET;

	return (toclear);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This function will remove the selected item from the widget */

int remove_selected_variable(const char *name)
{
	variable         *toclear;
	gchar            *string;

	g_assert(name != NULL);

#ifdef DEBUG
	g_message("%s(): variable: %s", __func__, name);
#endif
	/*
	 * Searching for the given variable.
	 */
	toclear = _tree_find(name, NULL);
	if (toclear == NULL)
		return -1;

	/* If the custom attribute "block-function-signals" is true
	 * then block signals whilst performing this function */
	if (toclear->widget_tag_attr &&
		((string = get_tag_attribute(toclear->widget_tag_attr, "block-function-signals"))) &&
		((strcasecmp(string, "true") == 0) || (strcasecmp(string, "yes") == 0) ||
		(atoi(string) == 1))) {
		GTKD_FUNCTION_SIGNALS_BLOCK;
	}

	/*
	 * Removing the selected item or text range from the widget.
	 */
	switch (toclear->Type) {
		case WIDGET_CANCELBUTTON:
		case WIDGET_HELPBUTTON:
		case WIDGET_NOBUTTON:
		case WIDGET_OKBUTTON:
		case WIDGET_YESBUTTON:
		case WIDGET_TOGGLEBUTTON:
		case WIDGET_BUTTON:
			widget_button_removeselected(toclear);
			break;
		case WIDGET_CHECKBOX:
			widget_checkbox_removeselected(toclear);
			break;
		case WIDGET_COLORBUTTON:
			widget_colorbutton_removeselected(toclear);
			break;
		case WIDGET_COMBOBOX:
			widget_combobox_removeselected(toclear);
			break;
		case WIDGET_COMBOBOXENTRY:
		case WIDGET_COMBOBOXTEXT:
			widget_comboboxtext_removeselected(toclear);
			break;
		case WIDGET_EDIT:
			widget_edit_removeselected(toclear);
			break;
		case WIDGET_ENTRY:
			widget_entry_removeselected(toclear);
			break;
		case WIDGET_EVENTBOX:
			widget_eventbox_removeselected(toclear);
			break;
		case WIDGET_EXPANDER:
			widget_expander_removeselected(toclear);
			break;
		case WIDGET_SWITCH:
			widget_switch_removeselected(toclear);
			break;
		case WIDGET_FILECHOOSER:
			widget_filechooser_removeselected(toclear);
			break;
		case WIDGET_CALENDAR:
			widget_calendar_removeselected(toclear);
			break;
		case WIDGET_LINKBUTTON:
			widget_linkbutton_removeselected(toclear);
			break;
		case WIDGET_SEARCHENTRY:
			widget_searchentry_removeselected(toclear);
			break;
		case WIDGET_INFOBAR:
			widget_infobar_removeselected(toclear);
			break;
		case WIDGET_SPINNER:
			widget_spinner_removeselected(toclear);
			break;
		case WIDGET_LEVELBAR:
			widget_levelbar_removeselected(toclear);
			break;
		case WIDGET_DRAWINGAREA:
			widget_drawingarea_removeselected(toclear);
			break;
		case WIDGET_IMAGE:
			widget_image_removeselected(toclear);
			break;
		case WIDGET_PULSE:
			widget_pulse_removeselected(toclear);
			break;
		case WIDGET_PASSWORD:
			widget_password_removeselected(toclear);
			break;
		case WIDGET_ASPECTFRAME:
			widget_aspectframe_removeselected(toclear);
			break;
		case WIDGET_FONTBUTTON:
			widget_fontbutton_removeselected(toclear);
			break;
		case WIDGET_FRAME:
			widget_frame_removeselected(toclear);
			break;
		case WIDGET_HBOX:
			widget_hbox_removeselected(toclear);
			break;
		case WIDGET_HSCALE:
		case WIDGET_VSCALE:
			widget_hscale_removeselected(toclear);
			break;
		case WIDGET_HSEPARATOR:
		case WIDGET_VSEPARATOR:
			widget_hseparator_removeselected(toclear);
			break;
		case WIDGET_LIST:
			widget_list_removeselected(toclear);
			break;
		case WIDGET_MENUBAR:
			widget_menubar_removeselected(toclear);
			break;
		case WIDGET_MENUITEMSEPARATOR:
		case WIDGET_MENUITEM:
		case WIDGET_MENU:
			widget_menuitem_removeselected(toclear);
			break;
		case WIDGET_NOTEBOOK:
			widget_notebook_removeselected(toclear);
			break;
		case WIDGET_PIXMAP:
			widget_pixmap_removeselected(toclear);
			break;
		case WIDGET_PROGRESSBAR:
			widget_progressbar_removeselected(toclear);
			break;
		case WIDGET_RADIOBUTTON:
			widget_radiobutton_removeselected(toclear);
			break;
		case WIDGET_SPINBUTTON:
			widget_spinbutton_removeselected(toclear);
			break;
		case WIDGET_STATUSBAR:
			widget_statusbar_removeselected(toclear);
			break;
		case WIDGET_TABLE:
			widget_table_removeselected(toclear);
			break;
		case WIDGET_TERMINAL:
			widget_terminal_removeselected(toclear);
			break;
		case WIDGET_TEXT:
			widget_text_removeselected(toclear);
			break;
		case WIDGET_TIMER:
			widget_timer_removeselected(toclear);
			break;
/* GTK3: always available - removed version guard */
		case WIDGET_TREE:
			widget_tree_removeselected(toclear);
			break;
		case WIDGET_VBOX:
			widget_vbox_removeselected(toclear);
			break;
		case WIDGET_WINDOW:
			widget_window_removeselected(toclear);
			break;
		default:
			g_warning("%s(): Delete not implemented for this widget.", __func__);
	}

	GTKD_FUNCTION_SIGNALS_RESET;

	return 0;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* This is a debug function we use to print all variables to the 
 * standard error. It will print a lot of information */

#ifdef DEBUG
void variables_print_debug(variable *actual)
{
	if (actual == NULL)
		actual = root;
	if (actual == NULL)
		return;

	if (actual->left != NULL)
		variables_print_debug(actual->left);

	variables_print_one(actual);

	if (actual->right != NULL)
		variables_print_debug(actual->right);
}
#endif
