/*
 * widgets.c: Widget manipulation functions
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
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

/* 
**
** $Id: widgets.c,v 1.2 2004/11/25 19:11:12 pipas Exp pipas $
** $Log: widgets.c,v $
** Revision 1.2  2004/11/25 19:11:12  pipas
** Small enhancements.
**
** Revision 1.1  2004/11/18 16:47:05  root
** Initial revision
**
 */

#include <gtk/gtk.h>
#include <math.h>
#include "gtkdialog.h"
#include "safe_exec.h"
#include "widgets.h"
#include "gtk4-events.h"
#include "stringman.h"
#include "widget_button.h"
#include "widget_checkbox.h"
#include "widget_colorbutton.h"
#include "widget_combobox.h"
#include "widget_comboboxtext.h"
#include "widget_edit.h"
#include "widget_entry.h"
#include "widget_eventbox.h"
#include "widget_expander.h"
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
/* GTK3 widgets portés en GTK4 */
#include "widget_switch.h"
#include "widget_calendar.h"
#include "widget_filechooser.h"
#include "widget_image.h"
#include "widget_infobar.h"
#include "widget_linkbutton.h"
#include "widget_password.h"
#include "widget_pulse.h"
#include "widget_searchentry.h"
#include "widget_spinner.h"
#include "widget_aspectframe.h"
/* Widgets GTK4-natifs */
#include "widget_vscale.h"
#include "widget_vseparator.h"
#include "widget_togglebutton.h"
#include "widget_levelbar.h"
#include "widget_revealer.h"
#include "widget_stack.h"
#include "widget_flowbox.h"
#include "widget_overlay.h"
#include "widget_drawingarea.h"
#include "signals.h"
#include "tag_attributes.h"
#include "macros.h"

/* Defines */
//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS
#undef DEBUG
#undef WARNING

extern gchar *option_include_file;

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

char *widget_get_text_value(GtkWidget *widget, int type)
{
	gchar            *string;

#ifdef DEBUG
	g_message("%s(): type: %08x\n", __func__, type);
#endif
	if (widget == NULL) {
		g_warning("%s(): NULL Widget", __func__);
		return ("NULL WIDGET");
	}

	switch (type) {
		case WIDGET_CANCELBUTTON:
		case WIDGET_HELPBUTTON:
		case WIDGET_NOBUTTON:
		case WIDGET_OKBUTTON:
		case WIDGET_YESBUTTON:
		case WIDGET_BUTTON:
			string = widget_button_envvar_construct(widget);
			return string;
			break;
		case WIDGET_TOGGLEBUTTON:
			string = widget_togglebutton_envvar_construct(widget);
			return string;
			break;
		case WIDGET_CHECKBOX:
			string = widget_checkbox_envvar_construct(widget);
			return string;
			break;
		case WIDGET_COLORBUTTON:
			string = widget_colorbutton_envvar_construct(widget);
			return string;
			break;
		case WIDGET_COMBOBOX:
			string = widget_combobox_envvar_construct(widget);
			return string;
			break;
		case WIDGET_COMBOBOXENTRY:
		case WIDGET_COMBOBOXTEXT:
			string = widget_comboboxtext_envvar_construct(widget);
			return string;
			break;
		case WIDGET_EDIT:
			string = widget_edit_envvar_construct(widget);
			return string;
			break;
		case WIDGET_ENTRY:
			string = widget_entry_envvar_construct(widget);
			return string;
			break;
		case WIDGET_EVENTBOX:
			string = widget_eventbox_envvar_construct(widget);
			return string;
			break;
		case WIDGET_EXPANDER:
			string = widget_expander_envvar_construct(widget);
			return string;
			break;
		case WIDGET_FONTBUTTON:
			string = widget_fontbutton_envvar_construct(widget);
			return string;
			break;
		case WIDGET_FRAME:
			string = widget_frame_envvar_construct(widget);
			return string;
			break;
		case WIDGET_HBOX:
			string = widget_hbox_envvar_construct(widget);
			return string;
			break;
		case WIDGET_HSCALE:
		case WIDGET_VSCALE:
			string = widget_hscale_envvar_construct(widget);
			return string;
			break;
		case WIDGET_HSEPARATOR:
		case WIDGET_VSEPARATOR:
			string = widget_hseparator_envvar_construct(widget);
			return string;
			break;
		case WIDGET_LIST:
			string = widget_list_envvar_construct(widget);
			return string;
			break;
		case WIDGET_MENUBAR:
			string = widget_menubar_envvar_construct(widget);
			return string;
			break;
		case WIDGET_MENUITEMSEPARATOR:
		case WIDGET_MENUITEM:
		case WIDGET_MENU:
			string = widget_menuitem_envvar_construct(widget);
			return string;
			break;
		case WIDGET_NOTEBOOK:
			string = widget_notebook_envvar_construct(widget);
			return string;
			break;
		case WIDGET_PIXMAP:
			string = widget_pixmap_envvar_construct(widget);
			return string;
			break;
		case WIDGET_PROGRESSBAR:
			string = widget_progressbar_envvar_construct(widget);
			return string;
			break;
		case WIDGET_RADIOBUTTON:
			string = widget_radiobutton_envvar_construct(widget);
			return string;
			break;
		case WIDGET_SPINBUTTON:
			string = widget_spinbutton_envvar_construct(widget);
			return string;
			break;
		case WIDGET_STATUSBAR:
			string = widget_statusbar_envvar_construct(widget);
			return string;
			break;
		case WIDGET_TABLE:
			string = widget_table_envvar_construct(widget);
			return string;
			break;
		case WIDGET_TERMINAL:
			string = widget_terminal_envvar_construct(widget);
			return string;
			break;
		case WIDGET_TEXT:
			string = widget_text_envvar_construct(widget);
			return string;
			break;
		case WIDGET_TIMER:
			string = widget_timer_envvar_construct(widget);
			return string;
			break;
		case WIDGET_TREE:
			string = widget_tree_envvar_construct(widget);
			return string;
			break;
		case WIDGET_VBOX:
			string = widget_vbox_envvar_construct(widget);
			return string;
			break;
		case WIDGET_WINDOW:
			string = widget_window_envvar_construct(widget);
			return string;
			break;
		/* GTK3 widgets portés en GTK4 */
		case WIDGET_SWITCH:
			string = widget_switch_envvar_construct(widget);
			return string;
			break;
		case WIDGET_CALENDAR:
			string = widget_calendar_envvar_construct(widget);
			return string;
			break;
		case WIDGET_FILECHOOSER:
			string = widget_filechooser_envvar_construct(widget);
			return string;
			break;
		case WIDGET_IMAGE:
			string = widget_image_envvar_construct(widget);
			return string;
			break;
		case WIDGET_INFOBAR:
			string = widget_infobar_envvar_construct(widget);
			return string;
			break;
		case WIDGET_LINKBUTTON:
			string = widget_linkbutton_envvar_construct(widget);
			return string;
			break;
		case WIDGET_PASSWORD:
			string = widget_password_envvar_construct(widget);
			return string;
			break;
		case WIDGET_PULSE:
			string = widget_pulse_envvar_construct(widget);
			return string;
			break;
		case WIDGET_SEARCHENTRY:
			string = widget_searchentry_envvar_construct(widget);
			return string;
			break;
		case WIDGET_SPINNER:
			string = widget_spinner_envvar_construct(widget);
			return string;
			break;
		case WIDGET_ASPECTFRAME:
			string = widget_aspectframe_envvar_construct(widget);
			return string;
			break;
		/* Widgets GTK4-natifs */
		case WIDGET_LEVELBAR:
			string = widget_levelbar_envvar_construct(widget);
			return string;
			break;
		case WIDGET_REVEALER:
			string = widget_revealer_envvar_construct(widget);
			return string;
			break;
		case WIDGET_STACK:
			string = widget_stack_envvar_construct(widget);
			return string;
			break;
		case WIDGET_FLOWBOX:
			string = widget_flowbox_envvar_construct(widget);
			return string;
			break;
		case WIDGET_OVERLAY:
			string = widget_overlay_envvar_construct(widget);
			return string;
			break;
		case WIDGET_DRAWINGAREA:
			string = widget_drawingarea_envvar_construct(widget);
			return string;
			break;
		default:
			return NULL;
	}
	g_error("%s(): this should not be reached", __func__);
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

/*
 * widget_opencommand:
 * Secure drop-in for the original popen()-based version.
 * Uses safe_popen() which runs the command via g_spawn (no /bin/sh -c),
 * preventing shell injection.  Returns FILE* so all 24+ existing callers
 * (fgets / pclose) work unchanged.
 */
FILE *widget_opencommand(const char *command)
{
	gchar *the_line = NULL;
	FILE  *infile;

	PIP_DEBUG("Opening command: '%s'", command);

	if (option_include_file != NULL) {
		the_line = g_strdup_printf(". %s; %s", option_include_file, command);
		infile = safe_popen(the_line);
		g_free(the_line);
	} else {
		infile = safe_popen(command);
	}

	if (infile == NULL)
		g_warning("%s(): failed to open command '%s'", __func__, command);

	return infile;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

char *widgets_to_str(int itype)
{
	char *type;
	switch (itype) {
		case WIDGET_CANCELBUTTON:
			type = "CANCELBUTTON";
			break;
		case WIDGET_HELPBUTTON:
			type = "HELPBUTTON";
			break;
		case WIDGET_NOBUTTON:
			type = "NOBUTTON";
			break;
		case WIDGET_OKBUTTON:
			type = "OKBUTTON";
			break;
		case WIDGET_YESBUTTON:
			type = "YESBUTTON";
			break;
		case WIDGET_BUTTON:
			type = "BUTTON";
			break;
		case WIDGET_CHECKBOX:
			type = "CHECKBOX";
			break;
		case WIDGET_COLORBUTTON:
			type = "COLORBUTTON";
			break;
		case WIDGET_COMBOBOX:
			type = "COMBOBOX";
			break;
		case WIDGET_COMBOBOXENTRY:
			type = "COMBOBOXENTRY";
			break;
		case WIDGET_COMBOBOXTEXT:
			type = "COMBOBOXTEXT";
			break;
		case WIDGET_EDIT:
			type = "EDIT";
			break;
		case WIDGET_ENTRY:
			type = "ENTRY";
			break;
		case WIDGET_EVENTBOX:
			type = "EVENTBOX";
			break;
		case WIDGET_EXPANDER:
			type = "EXPANDER";
			break;
		case WIDGET_FONTBUTTON:
			type = "FONTBUTTON";
			break;
		case WIDGET_FRAME:
			type = "FRAME";
			break;
		case WIDGET_HBOX:
			type = "HBOX";
			break;
		case WIDGET_HSCALE:
			type = "HSCALE";
			break;
		case WIDGET_HSEPARATOR:
			type = "HSEPARATOR";
			break;
		case WIDGET_LIST:
			type = "LIST";
			break;
		case WIDGET_MENU:
			type = "MENU";
			break;
		case WIDGET_MENUBAR:
			type = "MENUBAR";
			break;
		case WIDGET_MENUITEM:
			type = "MENUITEM";
			break;
		case WIDGET_MENUITEMSEPARATOR:
			type = "MENUITEMSEPARATOR";
			break;
		case WIDGET_NOTEBOOK:
			type = "NOTEBOOK";
			break;
		case WIDGET_PIXMAP:
			type = "PIXMAP";
			break;
		case WIDGET_PROGRESSBAR:
			type = "PROGRESSBAR";
			break;
		case WIDGET_RADIOBUTTON:
			type = "RADIOBUTTON";
			break;
		case WIDGET_SPINBUTTON:
			type = "SPINBUTTON";
			break;
		case WIDGET_STATUSBAR:
			type = "STATUSBAR";
			break;
		case WIDGET_TABLE:
			type = "TABLE";
			break;
		case WIDGET_TERMINAL:
			type = "TERMINAL";
			break;
		case WIDGET_TEXT:
			type = "TEXT";
			break;
		case WIDGET_TIMER:
			type = "TIMER";
			break;
		case WIDGET_TOGGLEBUTTON:
			type = "TOGGLEBUTTON";
			break;
		case WIDGET_TREE:
			type = "TREE";
			break;
		case WIDGET_VBOX:
			type = "VBOX";
			break;
		case WIDGET_VSCALE:
			type = "VSCALE";
			break;
		case WIDGET_VSEPARATOR:
			type = "VSEPARATOR";
			break;
		case WIDGET_WINDOW:
			type = "WINDOW";
			break;
		case WIDGET_SCROLLEDW:
			type = "SCROLLEDW";
			break;
		/* GTK3 widgets portés en GTK4 */
		case WIDGET_SWITCH:
			type = "SWITCH";
			break;
		case WIDGET_CALENDAR:
			type = "CALENDAR";
			break;
		case WIDGET_FILECHOOSER:
			type = "FILECHOOSER";
			break;
		case WIDGET_IMAGE:
			type = "IMAGE";
			break;
		case WIDGET_INFOBAR:
			type = "INFOBAR";
			break;
		case WIDGET_LINKBUTTON:
			type = "LINKBUTTON";
			break;
		case WIDGET_PASSWORD:
			type = "PASSWORD";
			break;
		case WIDGET_PULSE:
			type = "PULSE";
			break;
		case WIDGET_SEARCHENTRY:
			type = "SEARCHENTRY";
			break;
		case WIDGET_SPINNER:
			type = "SPINNER";
			break;
		case WIDGET_ASPECTFRAME:
			type = "ASPECTFRAME";
			break;
		/* Widgets GTK4-natifs */
		case WIDGET_LEVELBAR:
			type = "LEVELBAR";
			break;
		case WIDGET_REVEALER:
			type = "REVEALER";
			break;
		case WIDGET_STACK:
			type = "STACK";
			break;
		case WIDGET_FLOWBOX:
			type = "FLOWBOX";
			break;
		case WIDGET_OVERLAY:
			type = "OVERLAY";
			break;
		case WIDGET_DRAWINGAREA:
			type = "DRAWINGAREA";
			break;
		default:
			type = "THINGY";
	}
	return (type);
}

/***********************************************************************
 *  Widget Connect Signals                                             *
 ***********************************************************************/

gboolean widget_connect_signals(GtkWidget *widget, AttributeSet *Attr)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_return_val_if_fail(GTK_IS_WIDGET(widget), FALSE);

	PIP_DEBUG("Connecting signals for %p.", widget);

	/* "hide" and "show" survived GTK4 unchanged. */
	g_signal_connect(G_OBJECT(widget), "hide",
		G_CALLBACK(on_any_widget_hide), (gpointer)Attr);
	g_signal_connect(G_OBJECT(widget), "show",
		G_CALLBACK(on_any_widget_show), (gpointer)Attr);

	/* Everything else used to be a GtkWidget "*-event" signal.  GTK4
	 * deleted that whole family, so those connections were refused at
	 * runtime and the matching <action signal="..."> never fired.  The
	 * controllers below restore the behaviour under the same XML names.
	 * See gtk4-events.c, including what configure-event cannot become. */
	hp_gtk4_connect_widget_events(widget, Attr);
	hp_gtk4_connect_window_events(widget, Attr);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return TRUE;
}

/***********************************************************************
 * Widget Visibility List Add                                          *
 ***********************************************************************/
/* Add a widget to the hide list or to the show list */

void widget_visibility_list_add(GtkWidget *widget, tag_attr *attr)
{
	gchar            *value;
	gint              visible = TRUE;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (widget) {
		if (attr) {
			if ((value = get_tag_attribute(attr, "visible"))) {
				if ((strcasecmp(value, "false") == 0) ||
					(strcasecmp(value, "no") == 0) ||
					(strcasecmp(value, "0") == 0)) {
					visible = FALSE;
				}
			}
		}
		if (visible) {
			widget_show_list = g_list_append(widget_show_list, widget);
		} else {
			widget_hide_list = g_list_append(widget_hide_list, widget);
		}
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Widget Show All                                                     *
 ***********************************************************************/
/* Show all widgets individually.
 * 
 * Visible widgets are simply shown.
 * 
 * Invisible widgets are shown and then immediately hidden to allocate
 * resources else widgets like checkboxes won't function until shown.
 */

void widget_show_all(void)
{
	GList            *element;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Show visible widgets */
	if (widget_show_list) {
		element = widget_show_list;
		while (element) {
#ifdef DEBUG_CONTENT
			fprintf(stderr, "%s(): showing Name='%s'\n",
				__func__, find_variable_by_widget(element->data)->Name);
#endif
			gtk_widget_show(element->data);
			element = element->next;
		}
		/* Free the memory as we won't need it anymore */
		g_list_free(widget_show_list);
		/* This may be reused so being initially NULL is important */
		widget_show_list = NULL;
	}

	/* Show and then immediately hide invisible widgets */
	if (widget_hide_list) {
		element = widget_hide_list;
		while (element) {
#ifdef DEBUG_CONTENT
			fprintf(stderr, "%s(): showing then hiding Name='%s'\n",
				__func__, find_variable_by_widget(element->data)->Name);
#endif
			gtk_widget_show(element->data);
			gtk_widget_hide(element->data);
			element = element->next;
		}
		/* Free the memory as we won't need it anymore */
		g_list_free(widget_hide_list);
		/* This may be reused so being initially NULL is important */
		widget_hide_list = NULL;
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}
