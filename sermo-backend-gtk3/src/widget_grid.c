/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_grid.c — Conteneur de mise en page en tableau (GTK 3)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <grid columns="N"> range ses enfants EN FLOT : ordre du document, de gauche
 * à droite, retour à la ligne tous les N enfants. Ce que <hbox> dans <vbox> ne
 * sait pas faire : les colonnes sont ALIGNÉES d'une rangée à l'autre.
 *
 * Ce port est l'ÉTALON : c'est lui qui fabrique les .attendu du banc de
 * comportement. Les six autres se règlent sur ce qu'il rend.
 *
 * Attributs de balise :
 *   columns="N"           nombre de colonnes (défaut 1, avec avertissement)
 *   row-spacing="px"      espace entre les rangées
 *   column-spacing="px"   espace entre les colonnes
 *   homogeneous="true"    colonnes et rangées de taille égale
 * Par enfant (comme les boîtes) : space-expand / space-fill.
 *
 * ⚠️ <grid> n'est PAS <table> : <table> est la liste à colonnes héritée de
 * gtkdialog (des données), pas une mise en page.
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
#include "variables.h"
#include "widget_grid.h"
#include <string.h>
#include <stdlib.h>

/* Lecture d'un entier d'attribut de balise : renvoie `repli` si absent,
 * illisible ou négatif. */
static gint grid_attr_int(tag_attr *attr, const char *nom, gint repli)
{
	gchar *value;
	gint   n;

	if (!attr) return repli;
	value = get_tag_attribute(attr, nom);
	if (!value || !*value) return repli;
	n = atoi(value);
	return (n >= 0) ? n : repli;
}

static gboolean grid_attr_bool(tag_attr *attr, const char *nom)
{
	gchar *value;

	if (!attr) return FALSE;
	value = get_tag_attribute(attr, nom);
	if (!value || !*value) return FALSE;
	return (strcasecmp(value, "true") == 0 ||
	        strcasecmp(value, "yes") == 0 || atoi(value) == 1);
}

GtkWidget *widget_grid_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget    *widget;
	stackelement  s;
	gint          columns, n;
	gchar        *value;

	(void) Type;

	widget = gtk_grid_new();

	/* Nombre de colonnes. Absent : une seule colonne — la grille se comporte
	 * alors comme une pile, ce qui est le moins surprenant. Mais on le DIT :
	 * un <grid> sans columns= est presque toujours un oubli. */
	columns = grid_attr_int(attr, "columns", 0);
	if (columns <= 0) {
		if (!attr || !get_tag_attribute(attr, "columns"))
			g_warning("<grid> sans attribut columns= : une seule colonne. "
			          "Écrire par exemple <grid columns=\"2\">.");
		else
			g_warning("<grid columns=\"%s\"> : valeur inutilisable, "
			          "une seule colonne.",
			          get_tag_attribute(attr, "columns"));
		columns = 1;
	}

	gtk_grid_set_row_spacing(GTK_GRID(widget),
		grid_attr_int(attr, "row-spacing", 4));
	gtk_grid_set_column_spacing(GTK_GRID(widget),
		grid_attr_int(attr, "column-spacing", 8));
	if (grid_attr_bool(attr, "homogeneous")) {
		gtk_grid_set_row_homogeneous(GTK_GRID(widget), TRUE);
		gtk_grid_set_column_homogeneous(GTK_GRID(widget), TRUE);
	}

	/* Le cœur coalesce les enfants (instruction SUM) : UN SEUL pop, dans
	 * l'ordre du document. */
	s = pop();
	for (n = 0; n < s.nwidgets; ++n) {
		gint      ligne, colonne;
		gboolean  expand, fill;
		variable *var;

		if (!s.widgets[n]) continue;

		colonne = n % columns;
		ligne   = n / columns;

		/* Extensibilité : même règle que les boîtes — les zones de saisie et
		 * les conteneurs prennent la place, le reste garde sa taille — sauf
		 * space-expand / space-fill explicites sur l'enfant. Les attributs de
		 * l'enfant sont lisibles via sa variable (tout widget en a une, même
		 * auto-nommée), pas via la pile qui ne les transporte pas. */
		expand = fill = (s.widgettypes[n] == WIDGET_EDIT ||
		                 s.widgettypes[n] == WIDGET_FRAME ||
		                 s.widgettypes[n] == WIDGET_SCROLLEDW ||
		                 s.widgettypes[n] == WIDGET_ENTRY);

		var = find_variable_by_widget(s.widgets[n]);
		if (var && var->widget_tag_attr) {
			if ((value = get_tag_attribute(var->widget_tag_attr, "space-expand")))
				expand = (strcasecmp(value, "true") == 0 ||
				          strcasecmp(value, "yes") == 0 || atoi(value) == 1);
			if ((value = get_tag_attribute(var->widget_tag_attr, "space-fill")))
				fill = (strcasecmp(value, "true") == 0 ||
				        strcasecmp(value, "yes") == 0 || atoi(value) == 1);
		}

		gtk_widget_set_hexpand(s.widgets[n], expand);
		gtk_widget_set_halign(s.widgets[n], fill ? GTK_ALIGN_FILL : GTK_ALIGN_START);

		gtk_grid_attach(GTK_GRID(widget), s.widgets[n], colonne, ligne, 1, 1);
	}

	/* Ce widget a des enfants : ils demandent à être enregistrés pour la
	 * gestion de visibilité, comme pour frame et les boîtes. */
	widget_visibility_list_add(widget, attr);

	return widget;
}

/* Un conteneur n'a pas de valeur propre : chaîne VIDE, comme <eventbox> et
 * <frame> côté étalon. */
gchar *widget_grid_envvar_construct(GtkWidget *widget)
{
	(void) widget;
	return g_strdup("");
}
gchar *widget_grid_envvar_all_construct(variable *var)
{
	if (!var || !var->Widget) return NULL;
	return widget_grid_envvar_construct(var->Widget);
}
void widget_grid_clear(variable *var)          { (void) var; }
void widget_grid_refresh(variable *var)        { (void) var; }
void widget_grid_fileselect(variable *var, const char *name, const char *value)
{	(void) var; (void) name; (void) value; }
void widget_grid_removeselected(variable *var) { (void) var; }
void widget_grid_save(variable *var)           { (void) var; }
