/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_flowbox.c — Rangement automatique en lignes (GTK 3)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkFlowBox existe depuis GTK 3.12 : ce port n'avait simplement jamais été
 * écrit. Ce fichier est l'ÉTALON du tag — il fixe la valeur exportée, reprise
 * de l'implémentation gtk4 déjà présente : l'INDEX de l'enfant sélectionné,
 * chaîne vide s'il n'y en a pas.
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
#include "widget_flowbox.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_flowbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget        *widget;
	stackelement      s;
	gchar            *v;
	guint             min_par_ligne = 1, max_par_ligne = 4;
	gint              esp_col = 6, esp_lig = 6, n;
	GtkSelectionMode  selection = GTK_SELECTION_NONE;

	(void) Attr; (void) Type;

	if (attr) {
		if ((v = get_tag_attribute(attr, "min-children-per-line"))) min_par_ligne = (guint) atoi(v);
		if ((v = get_tag_attribute(attr, "max-children-per-line"))) max_par_ligne = (guint) atoi(v);
		if ((v = get_tag_attribute(attr, "column-spacing")))        esp_col = atoi(v);
		if ((v = get_tag_attribute(attr, "row-spacing")))           esp_lig = atoi(v);
		if ((v = get_tag_attribute(attr, "selection-mode"))) {
			if      (strcasecmp(v, "single")   == 0) selection = GTK_SELECTION_SINGLE;
			else if (strcasecmp(v, "browse")   == 0) selection = GTK_SELECTION_BROWSE;
			else if (strcasecmp(v, "multiple") == 0) selection = GTK_SELECTION_MULTIPLE;
		}
	}

	widget = gtk_flow_box_new();
	gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(widget), min_par_ligne);
	gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(widget), max_par_ligne);
	gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(widget), esp_col);
	gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(widget), esp_lig);
	gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(widget), selection);

	s = pop();
	for (n = 0; n < s.nwidgets; ++n) {
		if (!s.widgets[n]) continue;
		gtk_container_add(GTK_CONTAINER(widget), s.widgets[n]);
	}

	widget_visibility_list_add(widget, attr);
	return widget;
}

/* Export : l'index de l'enfant sélectionné, vide s'il n'y en a pas. */
gchar *widget_flowbox_envvar_construct(GtkWidget *widget)
{
	GList *choisis;
	gchar *resultat;

	if (!widget || !GTK_IS_FLOW_BOX(widget)) return g_strdup("");
	choisis = gtk_flow_box_get_selected_children(GTK_FLOW_BOX(widget));
	if (!choisis) return g_strdup("");
	resultat = g_strdup_printf("%d",
		gtk_flow_box_child_get_index(GTK_FLOW_BOX_CHILD(choisis->data)));
	g_list_free(choisis);
	return resultat;
}
gchar *widget_flowbox_envvar_all_construct(variable *var)
{
	if (!var || !var->Widget) return NULL;
	return widget_flowbox_envvar_construct(var->Widget);
}
void widget_flowbox_clear(variable *var)
{
	if (var && var->Widget && GTK_IS_FLOW_BOX(var->Widget))
		gtk_flow_box_unselect_all(GTK_FLOW_BOX(var->Widget));
}
void widget_flowbox_refresh(variable *var)        { (void) var; }
void widget_flowbox_fileselect(variable *var, const char *name, const char *value)
{	(void) var; (void) name; (void) value; }
void widget_flowbox_removeselected(variable *var) { (void) var; }
void widget_flowbox_save(variable *var)           { (void) var; }
