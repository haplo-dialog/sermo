/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubutton.c — Bouton qui déroule un menu (GTK 3)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkMenuButton + GtkMenu, garnie des GtkMenuItem que <menuitem> fabrique
 * déjà : le tag ne réinvente rien. Ce port est l'ÉTALON — il fixe la valeur
 * exportée : le libellé du DERNIER élément choisi.
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
#include "widget_menubutton.h"
#include <string.h>
#include <stdlib.h>

/* Le choix est retenu sur le BOUTON (« sermo_choix ») : c'est lui que le
 * dialogue manipule et que l'export lit. */
static void menubutton_choisi(GtkMenuItem *item, gpointer bouton)
{
	const gchar *lbl = gtk_menu_item_get_label(item);

	g_object_set_data_full(G_OBJECT(bouton), "sermo_choix",
	                       g_strdup(lbl ? lbl : ""), g_free);
}

GtkWidget *widget_menubutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget    *bouton, *menu;
	stackelement  s;
	GList        *element;
	gchar        *label = NULL;
	gint          n;

	(void) Type;

	if (Attr && attributeset_is_avail(Attr, ATTR_LABEL))
		label = attributeset_get_first(&element, Attr, ATTR_LABEL);
	if (!label && attr) label = get_tag_attribute(attr, "label");

	bouton = gtk_menu_button_new();
	/* ⚠️ gtk_container_add() sur un GtkMenuButton ne met RIEN : il a déjà son
	 * enfant (la flèche). Le bouton restait vide à l'écran, mesuré en capture.
	 * GtkMenuButton dérive de GtkButton : set_label est la bonne porte. */
	if (label && *label)
		gtk_button_set_label(GTK_BUTTON(bouton), label);

	menu = gtk_menu_new();
	s = pop();
	for (n = 0; n < s.nwidgets; ++n) {
		if (!s.widgets[n]) continue;
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), s.widgets[n]);
		if (GTK_IS_MENU_ITEM(s.widgets[n]))
			g_signal_connect(s.widgets[n], "activate",
			                 G_CALLBACK(menubutton_choisi), bouton);
	}
	gtk_widget_show_all(menu);
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(bouton), menu);

	return bouton;
}

/* Export : le libellé du dernier élément choisi, vide avant tout choix. */
gchar *widget_menubutton_envvar_construct(GtkWidget *widget)
{
	const gchar *choix;

	if (!widget) return g_strdup("");
	choix = (const gchar *) g_object_get_data(G_OBJECT(widget), "sermo_choix");
	return g_strdup(choix ? choix : "");
}
gchar *widget_menubutton_envvar_all_construct(variable *var)
{
	if (!var || !var->Widget) return NULL;
	return widget_menubutton_envvar_construct(var->Widget);
}
void widget_menubutton_clear(variable *var)
{
	if (var && var->Widget)
		g_object_set_data(G_OBJECT(var->Widget), "sermo_choix", NULL);
}
void widget_menubutton_refresh(variable *var)        { (void) var; }
void widget_menubutton_fileselect(variable *var, const char *name, const char *value)
{	(void) var; (void) name; (void) value; }
void widget_menubutton_removeselected(variable *var) { (void) var; }
void widget_menubutton_save(variable *var)           { (void) var; }
