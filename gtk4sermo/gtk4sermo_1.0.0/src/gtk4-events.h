/*
 * gtk4-events.h — GTK3 event signals rebuilt on GTK4 event controllers
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026  Haplo-Linux <devel@haplo-dialog.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _GTK4_EVENTS_H_
#define _GTK4_EVENTS_H_

#include <gtk/gtk.h>
#include "attributes.h"

/*
 * Attach the GTK4 event controllers that stand in for the GTK3 signals
 * button-press-event, button-release-event, key-press-event,
 * key-release-event, enter-notify-event, leave-notify-event,
 * focus-in-event, focus-out-event, map-event and unmap-event.
 *
 * The XML-facing signal names are unchanged: a script that says
 * <action signal="button-press-event"> keeps working.
 */
void hp_gtk4_connect_widget_events(GtkWidget *widget, AttributeSet *Attr);

/*
 * Attach the window-level stand-ins: close-request for delete-event and
 * destroy for destroy-event.  Safe to call on a non-window widget, in
 * which case only the destroy part is attached.
 */
void hp_gtk4_connect_window_events(GtkWidget *widget, AttributeSet *Attr);

#endif	/* _GTK4_EVENTS_H_ */
