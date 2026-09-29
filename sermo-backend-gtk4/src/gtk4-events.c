/*
 * gtk4-events.c — GTK3 event signals rebuilt on GTK4 event controllers
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026  haplo-dialog <devel@haplo-dialog.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * GTK4 removed the whole GtkWidget "*-event" signal family.  Measured on
 * GTK 4.22.4, every one of these is gone:
 *
 *   button-press-event   button-release-event  key-press-event
 *   key-release-event    enter-notify-event    leave-notify-event
 *   focus-in-event       focus-out-event       map-event
 *   unmap-event          configure-event       delete-event
 *   destroy-event
 *
 * The port kept connecting them, so GLib refused every connection with a
 * CRITICAL at startup and <action signal="button-press-event"> silently
 * did nothing.  GTK4 replaces them with event controllers, which this file
 * attaches instead.
 *
 * The XML dialect is UNCHANGED on purpose: the adapters below call
 * widget_signal_executor() with the historical GTK3 signal names, so a
 * dialog written for gtkdialog or gtk3sermo behaves the same here.
 *
 *
 * WHAT DOES NOT CARRY OVER
 * ------------------------
 * configure-event has no GTK4 equivalent and is not emulated.  It reported
 * window position and size; GTK4 does not expose window position at all,
 * because Wayland does not give it to the client.
 *
 * PTR_X_ROOT / PTR_Y_ROOT used to be screen-absolute pointer coordinates.
 * Those do not exist under Wayland either.  They are now relative to the
 * toplevel window, which is the closest honest equivalent, and they match
 * PTR_X / PTR_Y when the widget is the toplevel.
 */

#include <gtk/gtk.h>
#include <wchar.h>
#include "config.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "signals.h"
#include "gtk4-events.h"

/* ------------------------------------------------------------------ *
 * Helpers                                                            *
 * ------------------------------------------------------------------ */

/*
 * _controller_widget:
 * The widget an event controller is attached to.  The GTK3 callbacks took
 * the widget as their first argument; the controller signals do not, so we
 * ask the controller.
 */
static GtkWidget *_controller_widget(GtkEventController *controller)
{
	return gtk_event_controller_get_widget(controller);
}

/*
 * _toplevel_point:
 * Translate widget-relative (x, y) into coordinates relative to the
 * toplevel.  Replaces the GTK3 x_root / y_root, which were screen-absolute
 * and have no GTK4 equivalent.  Falls back to the input values when the
 * widget is not yet in a hierarchy.
 */
static void _toplevel_point(GtkWidget *widget, gdouble x, gdouble y,
	gdouble *out_x, gdouble *out_y)
{
	GtkRoot          *root = gtk_widget_get_root(widget);
	graphene_point_t  in, out;

	*out_x = x;
	*out_y = y;

	if (root == NULL)
		return;

	in.x = (float)x;
	in.y = (float)y;

	if (gtk_widget_compute_point(widget, GTK_WIDGET(root), &in, &out)) {
		*out_x = (gdouble)out.x;
		*out_y = (gdouble)out.y;
	}
}

/*
 * _run_pointer_signal:
 * Shared body of the press and release adapters.  Publishes the same
 * environment variables the GTK3 callbacks published, runs the action, then
 * removes them again.
 */
static void _run_pointer_signal(GtkGestureClick *gesture, gdouble x, gdouble y,
	AttributeSet *Attr, const gchar *signal_name)
{
	GtkEventController *controller = GTK_EVENT_CONTROLLER(gesture);
	GtkWidget          *widget     = _controller_widget(controller);
	GdkModifierType     state;
	guint               button;
	gdouble             rx, ry;
	gchar              *ptrx, *ptry, *ptrmod, *btn, *ptrbtn, *ptrxroot, *ptryroot;

	if (widget == NULL)
		return;

	state  = gtk_event_controller_get_current_event_state(controller);
	button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
	_toplevel_point(widget, x, y, &rx, &ry);

	ptrx     = g_strdup_printf("%.0f", x);
	ptry     = g_strdup_printf("%.0f", y);
	ptrmod   = g_strdup_printf("%u", (guint)state);
	btn      = g_strdup_printf("%u", button);
	ptrbtn   = g_strdup_printf("%u", button);
	ptrxroot = g_strdup_printf("%.0f", rx);
	ptryroot = g_strdup_printf("%.0f", ry);

	g_setenv("PTR_X", ptrx, TRUE);
	g_setenv("PTR_Y", ptry, TRUE);
	g_setenv("PTR_MOD", ptrmod, TRUE);
	g_setenv("BUTTON", btn, TRUE);		/* Deprecated */
	g_setenv("PTR_BTN", ptrbtn, TRUE);
	g_setenv("PTR_X_ROOT", ptrxroot, TRUE);
	g_setenv("PTR_Y_ROOT", ptryroot, TRUE);

	widget_signal_executor(widget, Attr, signal_name);

	g_unsetenv("PTR_X");
	g_unsetenv("PTR_Y");
	g_unsetenv("PTR_MOD");
	g_unsetenv("BUTTON");
	g_unsetenv("PTR_BTN");
	g_unsetenv("PTR_X_ROOT");
	g_unsetenv("PTR_Y_ROOT");

	g_free(ptrx);
	g_free(ptry);
	g_free(ptrmod);
	g_free(btn);
	g_free(ptrbtn);
	g_free(ptrxroot);
	g_free(ptryroot);
}

/*
 * _run_key_signal:
 * Shared body of the key press and release adapters.
 */
static gboolean _run_key_signal(GtkEventControllerKey *controller, guint keyval,
	guint keycode, GdkModifierType state, AttributeSet *Attr,
	const gchar *signal_name)
{
	GtkWidget *widget = _controller_widget(GTK_EVENT_CONTROLLER(controller));
	gchar     *kval, *ksym, *kuni, *kmod, *kraw;
	const gchar *name;

	if (widget == NULL)
		return FALSE;

	name = gdk_keyval_name(keyval);

	kval = g_strdup_printf("0x%03x", keyval);
	ksym = g_strdup(name ? name : "");
	kuni = g_strdup_printf("%lc", (wint_t)gdk_keyval_to_unicode(keyval));
	kmod = g_strdup_printf("%u", (guint)state);
	kraw = g_strdup_printf("0x%x", keycode);

	g_setenv("KEY_VAL", kval, TRUE);
	g_setenv("KEY_SYM", ksym, TRUE);
	g_setenv("KEY_UNI", kuni, TRUE);
	g_setenv("KEY_MOD", kmod, TRUE);
	g_setenv("KEY_RAW", kraw, TRUE);

	widget_signal_executor(widget, Attr, signal_name);

	g_unsetenv("KEY_VAL");
	g_unsetenv("KEY_SYM");
	g_unsetenv("KEY_UNI");
	g_unsetenv("KEY_MOD");
	g_unsetenv("KEY_RAW");

	g_free(kval);
	g_free(ksym);
	g_free(kuni);
	g_free(kmod);
	g_free(kraw);

	/* FALSE: do not swallow the key, let it keep propagating as GTK3 did. */
	return FALSE;
}

/*
 * _run_plain_signal:
 * For the signals that carried no data of their own.
 */
static void _run_plain_signal(GtkWidget *widget, AttributeSet *Attr,
	const gchar *signal_name)
{
	if (widget != NULL)
		widget_signal_executor(widget, Attr, signal_name);
}

/* ------------------------------------------------------------------ *
 * Adapters                                                           *
 * ------------------------------------------------------------------ */

static void _on_pressed(GtkGestureClick *gesture, gint n_press, gdouble x,
	gdouble y, gpointer data)
{
	(void)n_press;
	_run_pointer_signal(gesture, x, y, (AttributeSet *)data,
		"button-press-event");
}

static void _on_released(GtkGestureClick *gesture, gint n_press, gdouble x,
	gdouble y, gpointer data)
{
	(void)n_press;
	_run_pointer_signal(gesture, x, y, (AttributeSet *)data,
		"button-release-event");
}

static gboolean _on_key_pressed(GtkEventControllerKey *c, guint keyval,
	guint keycode, GdkModifierType state, gpointer data)
{
	return _run_key_signal(c, keyval, keycode, state, (AttributeSet *)data,
		"key-press-event");
}

static void _on_key_released(GtkEventControllerKey *c, guint keyval,
	guint keycode, GdkModifierType state, gpointer data)
{
	_run_key_signal(c, keyval, keycode, state, (AttributeSet *)data,
		"key-release-event");
}

static void _on_motion_enter(GtkEventControllerMotion *c, gdouble x, gdouble y,
	gpointer data)
{
	(void)x; (void)y;
	_run_plain_signal(_controller_widget(GTK_EVENT_CONTROLLER(c)),
		(AttributeSet *)data, "enter-notify-event");
}

static void _on_motion_leave(GtkEventControllerMotion *c, gpointer data)
{
	_run_plain_signal(_controller_widget(GTK_EVENT_CONTROLLER(c)),
		(AttributeSet *)data, "leave-notify-event");
}

static void _on_focus_enter(GtkEventControllerFocus *c, gpointer data)
{
	_run_plain_signal(_controller_widget(GTK_EVENT_CONTROLLER(c)),
		(AttributeSet *)data, "focus-in-event");
}

static void _on_focus_leave(GtkEventControllerFocus *c, gpointer data)
{
	_run_plain_signal(_controller_widget(GTK_EVENT_CONTROLLER(c)),
		(AttributeSet *)data, "focus-out-event");
}

static void _on_map(GtkWidget *widget, gpointer data)
{
	_run_plain_signal(widget, (AttributeSet *)data, "map-event");
}

static void _on_unmap(GtkWidget *widget, gpointer data)
{
	_run_plain_signal(widget, (AttributeSet *)data, "unmap-event");
}

static gboolean _on_close_request(GtkWindow *window, gpointer data)
{
	_run_plain_signal(GTK_WIDGET(window), (AttributeSet *)data,
		"delete-event");
	/* FALSE: let the default handler close the window, as GTK3 did when
	 * the delete-event handler returned FALSE. */
	return FALSE;
}

static void _on_destroy(GtkWidget *widget, gpointer data)
{
	_run_plain_signal(widget, (AttributeSet *)data, "destroy-event");
}

/* ------------------------------------------------------------------ *
 * Public entry points                                                *
 * ------------------------------------------------------------------ */

void hp_gtk4_connect_widget_events(GtkWidget *widget, AttributeSet *Attr)
{
	GtkEventController *click, *key, *motion, *focus;

	g_return_if_fail(GTK_IS_WIDGET(widget));

	/* Pointer buttons.  Button 0 means "any button", which is what the
	 * GTK3 button-press-event delivered. */
	click = GTK_EVENT_CONTROLLER(gtk_gesture_click_new());
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), 0);
	gtk_event_controller_set_propagation_phase(click, GTK_PHASE_BUBBLE);
	g_signal_connect(click, "pressed", G_CALLBACK(_on_pressed), Attr);
	g_signal_connect(click, "released", G_CALLBACK(_on_released), Attr);
	gtk_widget_add_controller(widget, click);

	/* Keyboard.  CAPTURE, not BUBBLE: an editable widget swallows the key
	 * in its own handler and returns TRUE, which ends the bubble before a
	 * controller on the GtkEntry itself ever sees it.  Measured: with
	 * BUBBLE, focus-in-event fired on an entry but key-press-event never
	 * did.  Capture runs from the root down to the target, so the same set
	 * of widgets is notified as GTK3 notified on the way up. */
	key = gtk_event_controller_key_new();
	gtk_event_controller_set_propagation_phase(key, GTK_PHASE_CAPTURE);
	g_signal_connect(key, "key-pressed", G_CALLBACK(_on_key_pressed), Attr);
	g_signal_connect(key, "key-released", G_CALLBACK(_on_key_released), Attr);
	gtk_widget_add_controller(widget, key);

	/* Pointer crossing. */
	motion = gtk_event_controller_motion_new();
	g_signal_connect(motion, "enter", G_CALLBACK(_on_motion_enter), Attr);
	g_signal_connect(motion, "leave", G_CALLBACK(_on_motion_leave), Attr);
	gtk_widget_add_controller(widget, motion);

	/* Keyboard focus. */
	focus = gtk_event_controller_focus_new();
	g_signal_connect(focus, "enter", G_CALLBACK(_on_focus_enter), Attr);
	g_signal_connect(focus, "leave", G_CALLBACK(_on_focus_leave), Attr);
	gtk_widget_add_controller(widget, focus);

	/* Mapping.  GTK4 kept these, it only dropped the "-event" suffix. */
	g_signal_connect(widget, "map", G_CALLBACK(_on_map), Attr);
	g_signal_connect(widget, "unmap", G_CALLBACK(_on_unmap), Attr);
}

void hp_gtk4_connect_window_events(GtkWidget *widget, AttributeSet *Attr)
{
	g_return_if_fail(GTK_IS_WIDGET(widget));

	if (GTK_IS_WINDOW(widget))
		g_signal_connect(widget, "close-request",
			G_CALLBACK(_on_close_request), Attr);

	g_signal_connect(widget, "destroy", G_CALLBACK(_on_destroy), Attr);
}
