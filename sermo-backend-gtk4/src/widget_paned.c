/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_paned.c — Deux zones et une poignée déplaçable (GTK 4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <paned> donne à l'UTILISATEUR le moyen de redistribuer la place entre deux
 * zones — liste à gauche, détail à droite. C'est la seule chose que le langage
 * ne savait pas faire : <hbox> fige le partage à l'écriture du script.
 *
 * Même widget qu'en GTK 3 (GtkPaned), mais l'API d'empaquetage a changé :
 * gtk_paned_pack1/pack2 ont disparu au profit de set_start_child/set_end_child,
 * et « resize » est devenu une propriété à part (set_resize_start_child).
 *
 * Attributs de balise :
 *   orientation="horizontal"  (défaut) deux zones côte à côte, poignée verticale
 *   orientation="vertical"    l'une au-dessus de l'autre
 *   position="240" | "30%"    position initiale de la poignée
 *   resizable="false"         poignée figée (les deux enfants gardent leur part)
 *
 * ⚠️ EXACTEMENT deux enfants. Un troisième est refusé AVEC un message : le
 * taire referait le défaut d'<eventbox>, qui perdait son contenu sans rien dire.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#include "config.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "variables.h"
#include "widget_paned.h"
#include <string.h>
#include <stdlib.h>

/* « 30% » ou « 240 ». Le pourcentage a besoin de la taille du conteneur, qui
 * n'existe pas encore à la création : on le garde et on l'applique quand la
 * fenêtre a sa taille (signal size-allocate, une seule fois). */
typedef struct {
	gdouble  fraction;   /* > 0 : part de la largeur/hauteur à donner au 1er */
	gboolean pose;       /* déjà appliqué ? */
} PanedFraction;

static void paned_pose_fraction(GtkWidget *widget, GtkAllocation *alloc, gpointer data)
{
	PanedFraction *pf = (PanedFraction *) data;
	gint           etendue;

	if (!pf || pf->pose) return;
	etendue = (gtk_orientable_get_orientation(GTK_ORIENTABLE(widget)) ==
	           GTK_ORIENTATION_HORIZONTAL) ? alloc->width : alloc->height;
	if (etendue <= 1) return;                 /* pas encore dimensionné */
	gtk_paned_set_position(GTK_PANED(widget), (gint) (etendue * pf->fraction));
	pf->pose = TRUE;
}

GtkWidget *widget_paned_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget      *widget;
	GtkOrientation  sens = GTK_ORIENTATION_HORIZONTAL;
	stackelement    s;
	gchar          *value;
	gboolean        redimensionnable = TRUE;
	gint            n, retenus = 0;

	(void) Type;

	if (attr && (value = get_tag_attribute(attr, "orientation"))) {
		if (strcasecmp(value, "vertical") == 0)
			sens = GTK_ORIENTATION_VERTICAL;
	}
	widget = gtk_paned_new(sens);

	if (attr && (value = get_tag_attribute(attr, "resizable"))) {
		redimensionnable = !(strcasecmp(value, "false") == 0 ||
		                     strcasecmp(value, "no") == 0 || strcmp(value, "0") == 0);
	}

	/* Les enfants arrivent coalescés par l'instruction SUM : UN SEUL pop. */
	s = pop();
	for (n = 0; n < s.nwidgets; ++n) {
		if (!s.widgets[n]) continue;
		if (retenus == 0) {
			gtk_paned_set_start_child(GTK_PANED(widget), s.widgets[n]);
			gtk_paned_set_resize_start_child(GTK_PANED(widget), redimensionnable);
		} else if (retenus == 1) {
			gtk_paned_set_end_child(GTK_PANED(widget), s.widgets[n]);
			gtk_paned_set_resize_end_child(GTK_PANED(widget), redimensionnable);
		} else {
			g_warning("<paned> prend EXACTEMENT deux enfants : le %d%s est "
			          "ignoré. Emballer le surplus dans une <vbox>.",
			          retenus + 1, retenus + 1 == 3 ? "e" : "e");
			continue;
		}
		retenus++;
	}
	if (retenus < 2)
		g_warning("<paned> n'a reçu que %d enfant(s) : la poignée n'a rien à "
		          "partager.", retenus);

	/* Position initiale : pixels tout de suite, pourcentage à l'allocation. */
	if (attr && (value = get_tag_attribute(attr, "position")) && *value) {
		gchar *fin = NULL;
		gdouble v = g_ascii_strtod(value, &fin);   /* ⚠️ jamais atof : la
		                                           * locale fr décide que
		                                           * « 0.3 » vaut zéro */
		if (fin && *fin == '%') {
			if (v > 0 && v < 100) {
				PanedFraction *pf = g_new0(PanedFraction, 1);
				pf->fraction = v / 100.0;
				g_signal_connect_data(widget, "size-allocate",
				                      G_CALLBACK(paned_pose_fraction), pf,
				                      (GClosureNotify) g_free, 0);
			}
		} else if (v >= 1) {
			gtk_paned_set_position(GTK_PANED(widget), (gint) v);
		}
	}

	/* Ce widget a des enfants : ils demandent à être enregistrés. */
	widget_visibility_list_add(widget, attr);

	return widget;
}

/* Un conteneur n'a pas de valeur propre : chaîne VIDE, comme <grid>, <frame>
 * et <eventbox> côté étalon. */
gchar *widget_paned_envvar_construct(GtkWidget *widget)
{
	(void) widget;
	return g_strdup("");
}
gchar *widget_paned_envvar_all_construct(variable *var)
{
	if (!var || !var->Widget) return NULL;
	return widget_paned_envvar_construct(var->Widget);
}
void widget_paned_clear(variable *var)          { (void) var; }
void widget_paned_refresh(variable *var)        { (void) var; }
void widget_paned_fileselect(variable *var, const char *name, const char *value)
{	(void) var; (void) name; (void) value; }
void widget_paned_removeselected(variable *var) { (void) var; }
void widget_paned_save(variable *var)           { (void) var; }
