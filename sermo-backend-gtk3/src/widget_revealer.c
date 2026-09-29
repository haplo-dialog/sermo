/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_revealer.c — Un enfant qui se montre et se cache (GTK 3)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkRevealer existe depuis GTK 3.10. Ce fichier est l'ÉTALON : export
 * « true » ou « false », l'état courant — repris de l'implémentation gtk4
 * déjà présente.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#include "config.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_revealer.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_revealer_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget                 *widget;
	stackelement               s;
	gchar                     *v;
	GtkRevealerTransitionType  transition = GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN;
	gint                       duree = 250, n, poses = 0;
	gboolean                   montre = FALSE;

	(void) Type;

	if (attr) {
		if ((v = get_tag_attribute(attr, "transition"))) {
			if      (strcasecmp(v, "none")        == 0) transition = GTK_REVEALER_TRANSITION_TYPE_NONE;
			else if (strcasecmp(v, "crossfade")   == 0) transition = GTK_REVEALER_TRANSITION_TYPE_CROSSFADE;
			else if (strcasecmp(v, "slide-right") == 0) transition = GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT;
			else if (strcasecmp(v, "slide-left")  == 0) transition = GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT;
			else if (strcasecmp(v, "slide-up")    == 0) transition = GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP;
		}
		if ((v = get_tag_attribute(attr, "duration"))) duree = atoi(v);
		if ((v = get_tag_attribute(attr, "reveal")) &&
		    (strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 || atoi(v) == 1))
			montre = TRUE;
	}
	/* <default>true</default> vaut aussi : c'est la forme habituelle du
	 * langage pour un état initial. */
	if (Attr && attributeset_is_avail(Attr, ATTR_DEFAULT)) {
		GList *el = NULL;
		gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
		if (def && (strcasecmp(def, "true") == 0 || strcasecmp(def, "yes") == 0 ||
		            atoi(def) == 1))
			montre = TRUE;
	}

	widget = gtk_revealer_new();
	gtk_revealer_set_transition_type(GTK_REVEALER(widget), transition);
	gtk_revealer_set_transition_duration(GTK_REVEALER(widget), duree);

	s = pop();
	for (n = 0; n < s.nwidgets; ++n) {
		if (!s.widgets[n]) continue;
		if (poses == 0) gtk_container_add(GTK_CONTAINER(widget), s.widgets[n]);
		else g_warning("<revealer> ne prend QU'UN enfant : le %de est ignoré. "
		               "Emballer le surplus dans une <vbox>.", poses + 1);
		poses++;
	}
	gtk_revealer_set_reveal_child(GTK_REVEALER(widget), montre);

	widget_visibility_list_add(widget, attr);
	return widget;
}

/* Export : l'état courant, « true » ou « false ». */
gchar *widget_revealer_envvar_construct(GtkWidget *widget)
{
	if (!widget || !GTK_IS_REVEALER(widget)) return g_strdup("false");
	return g_strdup(gtk_revealer_get_reveal_child(GTK_REVEALER(widget))
	                ? "true" : "false");
}
gchar *widget_revealer_envvar_all_construct(variable *var)
{
	if (!var || !var->Widget) return NULL;
	return widget_revealer_envvar_construct(var->Widget);
}
void widget_revealer_clear(variable *var)
{
	if (var && var->Widget && GTK_IS_REVEALER(var->Widget))
		gtk_revealer_set_reveal_child(GTK_REVEALER(var->Widget), FALSE);
}
void widget_revealer_refresh(variable *var)        { (void) var; }
void widget_revealer_fileselect(variable *var, const char *name, const char *value)
{	(void) var; (void) name; (void) value; }
void widget_revealer_removeselected(variable *var) { (void) var; }
void widget_revealer_save(variable *var)           { (void) var; }
