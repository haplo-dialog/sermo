/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_overlay.c — Enfants empilés l'un sur l'autre (GTK 3)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkOverlay existe depuis GTK 3.2. Le PREMIER enfant est le fond ; les
 * suivants flottent par-dessus. Ce fichier est l'ÉTALON : export vide, comme
 * tout conteneur.
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
#include "widget_overlay.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_overlay_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget    *widget;
	stackelement  s;
	gint          n, poses = 0;

	(void) Attr; (void) Type;

	widget = gtk_overlay_new();
	s = pop();
	for (n = 0; n < s.nwidgets; ++n) {
		if (!s.widgets[n]) continue;
		if (poses == 0) gtk_container_add(GTK_CONTAINER(widget), s.widgets[n]);
		else            gtk_overlay_add_overlay(GTK_OVERLAY(widget), s.widgets[n]);
		poses++;
	}
	if (poses < 2)
		g_warning("<overlay> n'a reçu que %d enfant(s) : il n'y a rien à "
		          "superposer.", poses);

	widget_visibility_list_add(widget, attr);
	return widget;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide. */
gchar *widget_overlay_envvar_construct(GtkWidget *widget)
{
	(void) widget;
	return g_strdup("");
}
gchar *widget_overlay_envvar_all_construct(variable *var)
{
	if (!var || !var->Widget) return NULL;
	return widget_overlay_envvar_construct(var->Widget);
}
void widget_overlay_clear(variable *var)          { (void) var; }
void widget_overlay_refresh(variable *var)        { (void) var; }
void widget_overlay_fileselect(variable *var, const char *name, const char *value)
{	(void) var; (void) name; (void) value; }
void widget_overlay_removeselected(variable *var) { (void) var; }
void widget_overlay_save(variable *var)           { (void) var; }
