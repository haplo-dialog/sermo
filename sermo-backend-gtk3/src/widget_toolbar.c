/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_toolbar.c — Barre d'actions (GTK 3)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ PAS de GtkToolbar. GTK 4 l'a RETIRÉ (aucun gtktoolbar.h dans
 * /usr/include/gtk-4.0) et GTK 3 le déprécie. Une GtkBox portant la classe
 * CSS « toolbar » donne le même rendu au thème, le MÊME code des deux côtés,
 * et aucune API condamnée. C'est la seule façon d'avoir un <toolbar>
 * identique sur gtk3 et gtk4 sans écrire deux implémentations.
 *
 * Export : chaîne vide — un conteneur n'a pas de valeur propre.
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
#include "widget_toolbar.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_toolbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget      *widget;
	GtkOrientation  sens = GTK_ORIENTATION_HORIZONTAL;
	stackelement    s;
	gchar          *value;
	gint            n, espacement = 2;

	(void) Attr; (void) Type;

	if (attr && (value = get_tag_attribute(attr, "orientation"))) {
		if (strcasecmp(value, "vertical") == 0)
			sens = GTK_ORIENTATION_VERTICAL;
	}
	if (attr && (value = get_tag_attribute(attr, "spacing")))
		espacement = atoi(value);

	widget = gtk_box_new(sens, espacement);
	/* La classe que les thèmes GTK connaissent déjà : bordure, fond, espacements
	 * d'une barre d'outils, sans qu'on peigne quoi que ce soit à la main. */
	gtk_style_context_add_class(gtk_widget_get_style_context(widget), "toolbar");

	s = pop();
	for (n = 0; n < s.nwidgets; ++n) {
		if (!s.widgets[n]) continue;
		gtk_box_pack_start(GTK_BOX(widget), s.widgets[n], FALSE, FALSE, 0);
	}

	widget_visibility_list_add(widget, attr);
	return widget;
}

/* Un conteneur n'a pas de valeur propre : chaîne VIDE, comme <grid> et <paned>. */
gchar *widget_toolbar_envvar_construct(GtkWidget *widget)
{
	(void) widget;
	return g_strdup("");
}
gchar *widget_toolbar_envvar_all_construct(variable *var)
{
	if (!var || !var->Widget) return NULL;
	return widget_toolbar_envvar_construct(var->Widget);
}
void widget_toolbar_clear(variable *var)          { (void) var; }
void widget_toolbar_refresh(variable *var)        { (void) var; }
void widget_toolbar_fileselect(variable *var, const char *name, const char *value)
{	(void) var; (void) name; (void) value; }
void widget_toolbar_removeselected(variable *var) { (void) var; }
void widget_toolbar_save(variable *var)           { (void) var; }
