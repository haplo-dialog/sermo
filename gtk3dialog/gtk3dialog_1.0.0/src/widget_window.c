/*
 * widget_window.c: 
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
#if HAVE_LAYER_SHELL
# include <errno.h>
# include <stdlib.h>
# include <string.h>
# include <gtk-layer-shell.h>
# ifdef GDK_WINDOWING_WAYLAND
#  include <gdk/gdkwayland.h>
# endif
#endif
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "tag_attributes.h"

/* Defines */
//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

extern gboolean option_centering;
extern gboolean have_geometry_xy;
extern gboolean have_geometry_dxdy;
extern gint geometry_dx;
extern gint geometry_dy;
extern gint geometry_x;
extern gint geometry_y;

/* Local function prototypes, located at file bottom */
static void widget_window_input_by_command(variable *var, char *command);
static void widget_window_input_by_file(variable *var, char *filename);
static void widget_window_input_by_items(variable *var);

/***********************************************************************
 * Auto-size on realize                                                *
 *                                                                     *
 * Fired once when the window is realized (before it appears).        *
 * Algorithm:                                                          *
 *   1. Ask GTK for the natural (preferred) size of the packed content.*
 *   2. If the caller already set a default size (via default-width /  *
 *      default-height attributes) respect it, but clamp it down when  *
 *      it exceeds the monitor work area.                              *
 *   3. If no default size was set, use the natural content size,      *
 *      again clamped to 95 % of the monitor work area.               *
 *                                                                     *
 * Result: the window always fits on screen and is never bigger than   *
 * its content requires.                                               *
 ***********************************************************************/
static void widget_window_on_realize(GtkWidget *widget, gpointer data)
{
	GdkDisplay    *display;
	GdkMonitor    *monitor;
	GdkRectangle   workarea;
	GtkRequisition natural;
	gint           dw, dh;
	gint           max_w, max_h;

	/* --- monitor work area ----------------------------------------- */
	display = gtk_widget_get_display(widget);
	monitor = gdk_display_get_primary_monitor(display);
	if (!monitor)
		monitor = gdk_display_get_monitor(display, 0);
	gdk_monitor_get_workarea(monitor, &workarea);
	max_w = (gint)(workarea.width  * 0.95);
	max_h = (gint)(workarea.height * 0.95);

	/* --- natural content size --------------------------------------- */
	gtk_widget_get_preferred_size(widget, NULL, &natural);

	/* --- current default size (-1 = unset) ------------------------- */
	gtk_window_get_default_size(GTK_WINDOW(widget), &dw, &dh);
	if (dw <= 0) dw = natural.width;
	if (dh <= 0) dh = natural.height;

	/* --- clamp to work area ---------------------------------------- */
	dw = MIN(dw, max_w);
	dh = MIN(dh, max_h);

	gtk_window_set_default_size(GTK_WINDOW(widget), dw, dh);
}

/* Notes: */

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_window_clear(variable *var)
{
	gchar            *var1;
	gint              var2;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Well, it can't be null because something changes it to
	 * "Unnamed Window", and since the default is PACKAGE when
	 * no title has been declared I'll clear it to that */

	gtk_window_set_title(GTK_WINDOW(var->Widget), PACKAGE);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/
#if HAVE_LAYER_SHELL

/* ---------------------------------------------------------------------------
 * wlr-layer-shell support (Wayland only)
 *
 * Four <window> tag attributes are recognised:
 *
 *   layer="background|bottom|top|overlay"
 *       Which layer of the compositor's stack the surface lives on.
 *
 *   edge="top|bottom|left|right|topleft|topright|bottomleft|bottomright|
 *         hstride|vstride|topstride|bottomstride|leftstride|rightstride"
 *       Which screen edge(s) the surface is anchored to.  The "stride"
 *       variants anchor two opposite edges so the surface stretches across
 *       the output, which is how one builds a bar or a dock.
 *
 *   dist="0..200"
 *       Margin in pixels between the surface and the edges it is anchored
 *       to.  Defaults to LAYER_SHELL_MARGIN_DEFAULT.
 *
 *   reserve="yes|no"
 *       Whether the compositor keeps the surface's space free.  The default,
 *       "no", lets the surface float over ordinary windows; "yes" reserves
 *       the size of the window plus its margin on the anchored edge, which
 *       is what a bar wants and what a dock usually does not.
 *
 * On X11, or on a Wayland compositor that does not implement
 * wlr-layer-shell, the attributes are ignored and the window is created as
 * an ordinary toplevel.
 *
 * Ported from the BunsenLabs gtk3dialog fork -- layer-shell support there
 * was written by Dima Krasner <dima@dimakrasner.com> (2021) and extended by
 * Mick Amadio <01micko@gmail.com> (2021-2024)
 * (https://github.com/BunsenLabs/gtk3dialog, src/widget_window.c, GPL-2+)
 * with two corrections to the original:
 *
 *   1. Upstream parsed dist= into a gushort and then range-checked it with
 *      "mar < 0" — a comparison that is always false for an unsigned type.
 *      An out-of-range dist= therefore wrapped around silently instead of
 *      falling back to the default.  We parse with strtol() into a signed
 *      long, reject trailing garbage, and clamp to [0, 200].
 *
 *   2. Upstream decided whether it was running under Wayland by reading the
 *      GDK_BACKEND environment variable, and proceeded whenever that
 *      variable was unset — which is the normal case under X11.
 *      gtk_layer_init_for_window() aborts the process when the display does
 *      not speak wlr-layer-shell, so an X11 session (or a GNOME/Mutter
 *      Wayland session, which does not implement the protocol either) could
 *      be killed outright by a dialog that merely carried a layer=
 *      attribute.  We ask the library instead, and skip the whole block when
 *      the answer is no.
 * ------------------------------------------------------------------------ */

#define LAYER_SHELL_MARGIN_DEFAULT   20
#define LAYER_SHELL_MARGIN_MAX      200

/* Sentinel meaning "the user did not ask for this". */
#define LAYER_SHELL_NO_LAYER  GTK_LAYER_SHELL_LAYER_ENTRY_NUMBER
#define LAYER_SHELL_NO_EDGE   GTK_LAYER_SHELL_EDGE_ENTRY_NUMBER

static const struct {
	const gchar        *name;
	GtkLayerShellLayer  layer;
} layer_shell_layers[] = {
	{ "background", GTK_LAYER_SHELL_LAYER_BACKGROUND },
	{ "bottom",     GTK_LAYER_SHELL_LAYER_BOTTOM     },
	{ "top",        GTK_LAYER_SHELL_LAYER_TOP        },
	{ "overlay",    GTK_LAYER_SHELL_LAYER_OVERLAY    },
};

/* Each entry anchors up to three edges: the primary one, plus a corner and
 * an opposite corner for the diagonal and "stride" forms. */
static const struct {
	const gchar       *name;
	GtkLayerShellEdge  edge;
	GtkLayerShellEdge  corner;
	GtkLayerShellEdge  oppcorner;
} layer_shell_edges[] = {
	{ "top",          GTK_LAYER_SHELL_EDGE_TOP,    LAYER_SHELL_NO_EDGE,          LAYER_SHELL_NO_EDGE          },
	{ "bottom",       GTK_LAYER_SHELL_EDGE_BOTTOM, LAYER_SHELL_NO_EDGE,          LAYER_SHELL_NO_EDGE          },
	{ "left",         GTK_LAYER_SHELL_EDGE_LEFT,   LAYER_SHELL_NO_EDGE,          LAYER_SHELL_NO_EDGE          },
	{ "right",        GTK_LAYER_SHELL_EDGE_RIGHT,  LAYER_SHELL_NO_EDGE,          LAYER_SHELL_NO_EDGE          },
	{ "topleft",      GTK_LAYER_SHELL_EDGE_LEFT,   GTK_LAYER_SHELL_EDGE_TOP,     LAYER_SHELL_NO_EDGE          },
	{ "bottomleft",   GTK_LAYER_SHELL_EDGE_LEFT,   GTK_LAYER_SHELL_EDGE_BOTTOM,  LAYER_SHELL_NO_EDGE          },
	{ "topright",     GTK_LAYER_SHELL_EDGE_RIGHT,  GTK_LAYER_SHELL_EDGE_TOP,     LAYER_SHELL_NO_EDGE          },
	{ "bottomright",  GTK_LAYER_SHELL_EDGE_RIGHT,  GTK_LAYER_SHELL_EDGE_BOTTOM,  LAYER_SHELL_NO_EDGE          },
	{ "hstride",      GTK_LAYER_SHELL_EDGE_RIGHT,  GTK_LAYER_SHELL_EDGE_LEFT,    LAYER_SHELL_NO_EDGE          },
	{ "vstride",      GTK_LAYER_SHELL_EDGE_TOP,    GTK_LAYER_SHELL_EDGE_BOTTOM,  LAYER_SHELL_NO_EDGE          },
	{ "topstride",    GTK_LAYER_SHELL_EDGE_TOP,    GTK_LAYER_SHELL_EDGE_LEFT,    GTK_LAYER_SHELL_EDGE_RIGHT   },
	{ "bottomstride", GTK_LAYER_SHELL_EDGE_BOTTOM, GTK_LAYER_SHELL_EDGE_LEFT,    GTK_LAYER_SHELL_EDGE_RIGHT   },
	{ "leftstride",   GTK_LAYER_SHELL_EDGE_LEFT,   GTK_LAYER_SHELL_EDGE_TOP,     GTK_LAYER_SHELL_EDGE_BOTTOM  },
	{ "rightstride",  GTK_LAYER_SHELL_EDGE_RIGHT,  GTK_LAYER_SHELL_EDGE_TOP,     GTK_LAYER_SHELL_EDGE_BOTTOM  },
};

/*
 * _layer_shell_supported:
 * TRUE when the current display actually speaks wlr-layer-shell.  Calling
 * gtk_layer_init_for_window() when this returns FALSE is fatal, so every
 * caller must check first.
 */
static gboolean _layer_shell_supported(void)
{
#if HAVE_LAYER_SHELL_IS_SUPPORTED
	/* gtk-layer-shell >= 0.9: the library answers for itself, and it
	 * accounts for Wayland compositors that lack the protocol. */
	return gtk_layer_is_supported();
#elif defined(GDK_WINDOWING_WAYLAND)
	{
		GdkDisplay *display = gdk_display_get_default();
		return display != NULL && GDK_IS_WAYLAND_DISPLAY(display);
	}
#else
	return FALSE;
#endif
}

/*
 * _layer_shell_parse_margin:
 * Parse the dist= attribute.  Returns the margin in pixels, or
 * LAYER_SHELL_MARGIN_DEFAULT when the value is missing, malformed or out of
 * range (a warning is emitted in the latter two cases).
 */
static gint _layer_shell_parse_margin(const gchar *value)
{
	gchar *end = NULL;
	long   margin;

	if (value == NULL || *value == '\0')
		return LAYER_SHELL_MARGIN_DEFAULT;

	errno = 0;
	margin = strtol(value, &end, 10);

	if (errno != 0 || end == value || *end != '\0') {
		g_warning("%s(): Malformed margin '%s'. Using default.",
			__func__, value);
		return LAYER_SHELL_MARGIN_DEFAULT;
	}

	if (margin < 0 || margin > LAYER_SHELL_MARGIN_MAX) {
		g_warning("%s(): Margin out of range '%s' (0-%d). Using default.",
			__func__, value, LAYER_SHELL_MARGIN_MAX);
		return LAYER_SHELL_MARGIN_DEFAULT;
	}

	return (gint)margin;
}

/*
 * _layer_shell_parse_reserve:
 * Parse the reserve= attribute.  Absent, empty or "no" means the surface
 * floats over ordinary windows, which is the historical behaviour; "yes" asks
 * the compositor to keep its space free.  An unrecognised value is reported
 * and treated as "no".
 */
static gboolean _layer_shell_parse_reserve(const gchar *value)
{
	if (value == NULL || *value == '\0')
		return FALSE;

	if (g_ascii_strcasecmp(value, "yes")  == 0 ||
	    g_ascii_strcasecmp(value, "true") == 0 ||
	    g_ascii_strcasecmp(value, "1")    == 0)
		return TRUE;

	if (g_ascii_strcasecmp(value, "no")    == 0 ||
	    g_ascii_strcasecmp(value, "false") == 0 ||
	    g_ascii_strcasecmp(value, "0")     == 0)
		return FALSE;

	g_warning("%s(): Unknown reserve '%s'. Using 'no'.", __func__, value);
	return FALSE;
}

/*
 * _layer_shell_apply:
 * Read layer=, edge= and dist= off the <window> tag and, when at least one
 * of layer= or edge= was given, turn the window into a layer-shell surface.
 * A no-op on a display without wlr-layer-shell.
 */
static void _layer_shell_apply(GtkWidget *widget, tag_attr *attr)
{
	GtkLayerShellLayer  layer     = LAYER_SHELL_NO_LAYER;
	GtkLayerShellEdge   edge      = LAYER_SHELL_NO_EDGE;
	GtkLayerShellEdge   corner    = LAYER_SHELL_NO_EDGE;
	GtkLayerShellEdge   oppcorner = LAYER_SHELL_NO_EDGE;
	gint                margin    = LAYER_SHELL_MARGIN_DEFAULT;
	gboolean            reserve   = FALSE;
	gchar              *value;
	gsize               i;

	if (attr == NULL)
		return;

	value = get_tag_attribute(attr, "layer");
	if (value) {
		for (i = 0; i < G_N_ELEMENTS(layer_shell_layers); i++) {
			if (strcmp(value, layer_shell_layers[i].name) == 0) {
				layer = layer_shell_layers[i].layer;
				break;
			}
		}
		if (layer == LAYER_SHELL_NO_LAYER)
			g_warning("%s(): Unknown layer '%s'.", __func__, value);
	}

	value = get_tag_attribute(attr, "edge");
	if (value) {
		for (i = 0; i < G_N_ELEMENTS(layer_shell_edges); i++) {
			if (strcmp(value, layer_shell_edges[i].name) == 0) {
				edge      = layer_shell_edges[i].edge;
				corner    = layer_shell_edges[i].corner;
				oppcorner = layer_shell_edges[i].oppcorner;
				break;
			}
		}
		if (edge == LAYER_SHELL_NO_EDGE)
			g_warning("%s(): Unknown edge '%s'.", __func__, value);
	}

	/* Nothing was asked for: leave the window as a plain toplevel. */
	if (layer == LAYER_SHELL_NO_LAYER && edge == LAYER_SHELL_NO_EDGE)
		return;

	/* Validate dist= even when the display cannot honour the request, so
	 * that a typo is reported while developing under X11. */
	margin = _layer_shell_parse_margin(get_tag_attribute(attr, "dist"));
	reserve = _layer_shell_parse_reserve(get_tag_attribute(attr, "reserve"));

	if (reserve && edge == LAYER_SHELL_NO_EDGE)
		g_warning("%s(): reserve= has no effect without edge=.", __func__);

	/* Asked for, but the display cannot honour it.  Say so once and carry
	 * on as an ordinary window. */
	if (!_layer_shell_supported()) {
		g_message("%s(): layer-shell requested but this display does not "
			"support it; falling back to a normal window.", __func__);
		return;
	}

	gtk_layer_init_for_window(GTK_WINDOW(widget));
	gtk_layer_set_namespace(GTK_WINDOW(widget), PACKAGE);

	if (layer != LAYER_SHELL_NO_LAYER)
		gtk_layer_set_layer(GTK_WINDOW(widget), layer);

	if (edge != LAYER_SHELL_NO_EDGE) {
		gtk_layer_set_exclusive_zone(GTK_WINDOW(widget), 0);
		gtk_layer_set_margin(GTK_WINDOW(widget), GTK_LAYER_SHELL_EDGE_LEFT,   margin);
		gtk_layer_set_margin(GTK_WINDOW(widget), GTK_LAYER_SHELL_EDGE_RIGHT,  margin);
		gtk_layer_set_margin(GTK_WINDOW(widget), GTK_LAYER_SHELL_EDGE_TOP,    margin);
		gtk_layer_set_margin(GTK_WINDOW(widget), GTK_LAYER_SHELL_EDGE_BOTTOM, margin);
		gtk_layer_set_anchor(GTK_WINDOW(widget), edge, TRUE);
		if (corner != LAYER_SHELL_NO_EDGE)
			gtk_layer_set_anchor(GTK_WINDOW(widget), corner, TRUE);
		if (oppcorner != LAYER_SHELL_NO_EDGE)
			gtk_layer_set_anchor(GTK_WINDOW(widget), oppcorner, TRUE);

		/* The exclusive zone was set to 0 above, so by default the
		 * surface floats over ordinary windows.  reserve="yes" asks
		 * for the opposite: the compositor keeps the window's size
		 * plus its margin on the anchored edge free, and lays other
		 * windows out beside it.  Enabled last, once the anchors it
		 * derives the zone from are in place. */
		if (reserve)
			gtk_layer_auto_exclusive_zone_enable(GTK_WINDOW(widget));
	}
}

#endif	/* HAVE_LAYER_SHELL */

/* ---------------------------------------------------------------------------
 * Client-side decorations (CSD)
 *
 *   headerbar="yes|no"
 *       "yes" makes the window draw its own title bar, a GtkHeaderBar, in
 *       place of the one the window manager would draw.  Default "no": the
 *       window manager keeps drawing it.
 *
 * This is deliberately opt-in and off by default.  Haplo ships XFCE, where a
 * client-drawn title bar stops matching the window-manager theme and the
 * window buttons stop matching every other window on the desktop.  A script
 * that wants the modern GNOME look asks for it; nothing gets it by surprise.
 *
 * Note the neighbouring attribute: decorated="false" removes the title bar
 * altogether.  The three cases are therefore
 *   (nothing)            window manager draws the title bar
 *   headerbar="yes"      the program draws it
 *   decorated="false"    nobody draws it
 *
 * The attribute name is not "titlebar" on purpose: GtkWindow in GTK4 has a
 * "titlebar" property of type GtkWidget, and tag_attributes.c applies any
 * attribute that matches a GObject property, so that name would be captured
 * and fed a string where a widget is expected.  "headerbar" is free in both
 * GTK3 and GTK4 (measured).
 * ------------------------------------------------------------------------ */

/*
 * _parse_yes_no:
 * Shared yes/no attribute parsing.  Unrecognised values are reported and
 * treated as the default given by def.
 */
static gboolean _parse_yes_no(const gchar *value, gboolean def, const gchar *what)
{
	if (value == NULL || *value == '\0')
		return def;

	if (g_ascii_strcasecmp(value, "yes")  == 0 ||
	    g_ascii_strcasecmp(value, "true") == 0 ||
	    g_ascii_strcasecmp(value, "1")    == 0)
		return TRUE;

	if (g_ascii_strcasecmp(value, "no")    == 0 ||
	    g_ascii_strcasecmp(value, "false") == 0 ||
	    g_ascii_strcasecmp(value, "0")     == 0)
		return FALSE;

	g_warning("%s(): Unknown %s '%s'. Using '%s'.", __func__, what, value,
		def ? "yes" : "no");
	return def;
}

/*
 * _apply_headerbar:
 * Replace the window-manager title bar by a GtkHeaderBar carrying the same
 * title and a close button.  Call it after gtk_window_set_title(), so the
 * header bar can pick the title up.
 */
static void _apply_headerbar(GtkWidget *window, tag_attr *attr)
{
	GtkWidget   *bar;
	const gchar *title;
	gchar       *value;

	if (attr == NULL)
		return;

	value = get_tag_attribute(attr, "headerbar");
	if (value == NULL)
		return;

	if (!_parse_yes_no(value, FALSE, "headerbar"))
		return;

	bar = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(bar), TRUE);

	/* The header bar does not inherit the window title, it has its own. */
	title = gtk_window_get_title(GTK_WINDOW(window));
	if (title != NULL)
		gtk_header_bar_set_title(GTK_HEADER_BAR(bar), title);

	gtk_window_set_titlebar(GTK_WINDOW(window), bar);
	gtk_widget_show_all(bar);
}

GtkWidget *widget_window_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	gchar            *value;
	GError           *error = NULL;
	GList            *accel_group = NULL;
	GList            *element;
	gint              border_width;
	GtkWidget        *widget;
	stackelement      s;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Create the window widget */
	widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);  

#if HAVE_LAYER_SHELL
	/* Wayland: honour layer=/edge=/dist= by turning the window into a
	 * wlr-layer-shell surface.  No-op elsewhere. */
	_layer_shell_apply(widget, attr);
#endif

	/* Set a default window title */
	attributeset_set_if_unset(Attr, ATTR_LABEL, PACKAGE);
	gtk_window_set_title(GTK_WINDOW(widget), 
		attributeset_get_first(&element, Attr, ATTR_LABEL));

	/* Client-side decorations, opt-in via headerbar="yes".  After the title
	 * is set, so the header bar can carry it. */
	_apply_headerbar(widget, attr);

	/* Set a default title bar theme icon */
	gtk_window_set_icon_name(GTK_WINDOW(widget), PACKAGE);

	/* If requested set a title bar image by filename */
	if (attr) {
		if ((value = get_tag_attribute(attr, "image-name")))
			gtk_window_set_icon_from_file(GTK_WINDOW(widget),
				find_pixmap(value), &error);
	}

	/* Set a default border width */
	border_width = 5;
	if (attr && (value = get_tag_attribute(attr, "margin")))	/* Deprecated */
		border_width = atoi(value);
	gtk_container_set_border_width(GTK_CONTAINER(widget), border_width);

	/* Tag attributes: resizable, default-width/height, width-request/height-request.
	 *
	 * default-width / default-height  → gtk_window_set_default_size()
	 *   The window opens at this size but GTK expands it automatically if
	 *   the content requires more space.  Use -1 to let GTK choose a dimension.
	 *   This is the recommended way to set an initial size.
	 *
	 * width-request / height-request  → gtk_widget_set_size_request()
	 *   Hard minimum: the window can never be smaller than these values.
	 *   Only set when you really need to enforce a minimum.
	 *
	 * resizable  (true/false, default true)
	 *   Controls whether the user can resize the window.
	 */
	if (attr) {
		gint dw = -1, dh = -1;
		if ((value = get_tag_attribute(attr, "default-width")))
			dw = atoi(value);
		if ((value = get_tag_attribute(attr, "default-height")))
			dh = atoi(value);
		if (dw != -1 || dh != -1)
			gtk_window_set_default_size(GTK_WINDOW(widget), dw, dh);

		if ((value = get_tag_attribute(attr, "width-request"))) {
			gint wr = atoi(value);
			gint hr = -1;
			if ((value = get_tag_attribute(attr, "height-request")))
				hr = atoi(value);
			gtk_widget_set_size_request(widget, wr, hr);
		} else if ((value = get_tag_attribute(attr, "height-request"))) {
			gtk_widget_set_size_request(widget, -1, atoi(value));
		}

		if ((value = get_tag_attribute(attr, "resizable")))
			gtk_window_set_resizable(GTK_WINDOW(widget),
				(g_ascii_strcasecmp(value, "false") != 0 &&
				 g_ascii_strcasecmp(value, "no")    != 0 &&
				 g_ascii_strcasecmp(value, "0")     != 0));
	}

	/* If we have geometry given in the command line, we set that */
	if (have_geometry_dxdy)
		gtk_widget_set_size_request(widget, geometry_dx, geometry_dy);
	if (have_geometry_xy)
		gtk_window_move(GTK_WINDOW(widget), geometry_x, geometry_y);
	if (option_centering)
		gtk_window_set_position(GTK_WINDOW(widget),
			GTK_WIN_POS_CENTER_ALWAYS);

	/* Auto-size: clamp to screen work area on realize (fires before show) */
	g_signal_connect(widget, "realize",
		G_CALLBACK(widget_window_on_realize), NULL);

	/* Pop the widgets that the window will contain and add them */
	s = pop();
	gtk_container_add(GTK_CONTAINER(widget), s.widgets[0]);

	/* Thunor: Each menu created will have an accelerator group
	 * for its menuitems which will require adding to the window */
	if (accel_groups) {
		accel_group = g_list_first(accel_groups);
		while (accel_group) {
			gtk_window_add_accel_group(GTK_WINDOW(widget),
				GTK_ACCEL_GROUP(accel_group->data));
#ifdef DEBUG
			fprintf(stderr, "%s: Adding accel_group=%p to window\n",
				__func__, accel_group->data);
#endif
			accel_group = accel_group->next;
		}
		g_list_free(accel_groups);
		accel_groups = NULL;
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_window_envvar_all_construct(variable *var)
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

gchar *widget_window_envvar_construct(GtkWidget *widget)
{
	gchar            *string;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Thunor: Variables are exported before a window is launched which
	 * can be a problem if launching from a launched window because
	 * launched windows are required to contain a variable that matches
	 * the name of the program's envvar and this results in the program
	 * being overwritten with the window title ;) It took me a day to
	 * find this issue because the error was being reported by the parser
	 * but the problem was located in action_launchwindow. So window
	 * variables can't be exported, they must be reserved for scripts.
	 * 
	 * string = g_strdup(gtk_window_get_title(GTK_WINDOW(widget))); */
	string = NULL;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_window_fileselect(
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
void widget_window_refresh(variable *var)
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
			widget_window_input_by_command(var, act + 8);
		/* input file stock = "File:", input file = "File:/path/to/file" */
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised) {
				/* Check for file-monitor and create if requested */
				widget_file_monitor_try_create(var, act + 5);
			}
			widget_window_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_window_input_by_items(var);

	/* Initialise these only once at start-up */
	if (!initialised) {
		/* Apply directives */
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT))
			g_warning( "%s(): <default> not implemented for this widget.",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_HEIGHT))
			g_warning( "%s(): <height> not implemented for this widget.",
				__func__);
		if (attributeset_is_avail(var->Attributes, ATTR_WIDTH))
			g_warning( "%s(): <width> not implemented for this widget.",
				__func__);
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||	/* Deprecated */
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);

		/* Connect signals */
		g_signal_connect(G_OBJECT(var->Widget), "delete-event",
			G_CALLBACK(window_delete_event_handler), NULL);

	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_window_removeselected(variable *var)
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

void widget_window_save(variable *var)
{
	FILE             *outfile;
	GList            *element;
	gchar            *act;
	gchar            *filename = NULL;

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
			fprintf(outfile, "%s", gtk_window_get_title(GTK_WINDOW(var->Widget)));
			/* Close the file */
			fclose(outfile);
		} else {
			g_warning( "%s(): Couldn't open '%s' for writing.",
				__func__, filename);
		}
	} else {
		g_warning( "%s(): No <output file> directive found.", __func__);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Command                                                    *
 ***********************************************************************/

static void widget_window_input_by_command(variable *var, char *command)
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

	/* Opening pipe for reading... */
	if ((infile = widget_opencommand(command))) {
		/* Just one line */
		if (fgets(line, 512, infile)) {
			/* Enforce end of string in case of max chars read */
			line[512 - 1] = 0;
			/* Remove the trailing [CR]LFs */
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;

			gtk_window_set_title(GTK_WINDOW(var->Widget), line);

		}
		/* Close the file */
		fclose(infile);
	} else {
		g_warning( "%s(): Couldn't open '%s' for reading.", __func__,
			command);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by File                                                       *
 ***********************************************************************/

static void widget_window_input_by_file(variable *var, char *filename)
{
	FILE             *infile;
	gchar             line[512];
	gint              count;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if ((infile = fopen(filename, "r"))) {
		/* Just one line */
		if (fgets(line, 512, infile)) {
			/* Enforce end of string in case of max chars read */
			line[512 - 1] = 0;
			/* Remove the trailing [CR]LFs */
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10) line[count] = 0;

			gtk_window_set_title(GTK_WINDOW(var->Widget), line);

		}
		/* Close the file */
		fclose(infile);
	} else {
		g_warning( "%s(): Couldn't open '%s' for reading.", __func__,
			filename);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Input by Items                                                      *
 ***********************************************************************/

static void widget_window_input_by_items(variable *var)
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
