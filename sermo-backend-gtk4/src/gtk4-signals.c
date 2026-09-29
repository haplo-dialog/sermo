/* SPDX-License-Identifier: GPL-2.0-or-later */
/* gtk4-signals.c — glue de signaux propre à GTK4 (backend gtk4).
 *
 * Le cœur (libsermocore) fournit les gestionnaires de la famille GObject
 * hérités de GTK3 (dont window_delete_event_handler). GTK4 a remplacé
 * « delete-event » par « close-request », émis avec un argument de moins ;
 * cet adaptateur — spécifique au toolkit, donc côté backend — recolle la
 * forme et défère au gestionnaire historique du cœur, laissé intact. */
#include <gtk/gtk.h>
#include "gtk4-compat.h"
#include "gtkdialog.h"
#include "signals.h"

gboolean window_close_request_handler(GtkWindow *window, gpointer data)
{
	return window_delete_event_handler(GTK_WIDGET(window), NULL, data);
}
