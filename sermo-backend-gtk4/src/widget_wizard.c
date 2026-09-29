/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_wizard.c — Suite d'étapes (GTK 4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ COMPOSÉ, pas emprunté : GTK 4 a RETIRÉ GtkAssistant (aucun
 * gtkassistant.h dans /usr/include/gtk-4.0). Un assistant, ici, c'est un
 * GtkStack plus trois boutons — le même montage des deux côtés, et le même
 * comportement que sur les cinq autres ports.
 *
 * « Terminer » joue l'<action> du wizard ; il ne ferme rien de lui-même.
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
#include "widget_wizard.h"
#include <string.h>
#include <stdlib.h>

/* execute_action vient de actions.h : une seule déclaration, sinon violation
 * de l'ODR (type de retour et constance divergents). */
#include "actions.h"

/* L'étape courante vit sur l'ENVELOPPE (« sermo_etape ») : c'est elle que le
 * dialogue manipule, et c'est elle que l'export lit. La pile GTK ne sert qu'à
 * l'affichage. */
static void wizard_aller_a(GtkWidget *hote, gint delta)
{
	GtkWidget *pile = (GtkWidget *) g_object_get_data(G_OBJECT(hote), "sermo_pile");
	gint       n    = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(hote), "sermo_n"));
	gint       cur  = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(hote), "sermo_etape"));
	gchar      nom[16];

	cur += delta;
	if (cur < 0) cur = 0;
	if (cur >= n) cur = n ? n - 1 : 0;
	g_object_set_data(G_OBJECT(hote), "sermo_etape", GINT_TO_POINTER(cur));
	g_snprintf(nom, sizeof(nom), "e%d", cur);
	if (pile) gtk_stack_set_visible_child_name(GTK_STACK(pile), nom);
}

static void wizard_precedent(GtkWidget *b, gpointer hote) { (void) b; wizard_aller_a(GTK_WIDGET(hote), -1); }
static void wizard_suivant(GtkWidget *b, gpointer hote)   { (void) b; wizard_aller_a(GTK_WIDGET(hote), +1); }

static void wizard_terminer(GtkWidget *b, gpointer donnee)
{
	AttributeSet *Attr = (AttributeSet *) donnee;
	GList        *el = NULL;
	gchar        *fn;

	if (!Attr) return;
	fn = attributeset_get_first(&el, Attr, ATTR_ACTION);
	while (fn) {
		if (*fn) execute_action(b, fn, NULL);
		fn = attributeset_get_next(&el, Attr, ATTR_ACTION);
	}
}

GtkWidget *widget_wizard_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget    *pile, *hote, *barre, *prec, *suiv, *fin;
	stackelement  s;
	gint          n, etapes = 0;

	(void) Type;

	pile = gtk_stack_new();
	s = pop();
	for (n = 0; n < s.nwidgets; ++n) {
		gchar nom[16];
		if (!s.widgets[n]) continue;
		g_snprintf(nom, sizeof(nom), "e%d", etapes);
		gtk_stack_add_named(GTK_STACK(pile), s.widgets[n], nom);
		etapes++;
	}

	hote  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	barre = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	prec  = gtk_button_new_with_label("Précédent");
	suiv  = gtk_button_new_with_label("Suivant");
	fin   = gtk_button_new_with_label("Terminer");

	g_object_set_data(G_OBJECT(hote), "sermo_pile", pile);
	g_object_set_data(G_OBJECT(hote), "sermo_n", GINT_TO_POINTER(etapes));
	g_object_set_data(G_OBJECT(hote), "sermo_etape", GINT_TO_POINTER(0));

	g_signal_connect(prec, "clicked", G_CALLBACK(wizard_precedent), hote);
	g_signal_connect(suiv, "clicked", G_CALLBACK(wizard_suivant), hote);
	g_signal_connect(fin,  "clicked", G_CALLBACK(wizard_terminer), Attr);

	gtk_box_append(GTK_BOX(barre), prec);
	gtk_box_append(GTK_BOX(barre), suiv);
	gtk_box_append(GTK_BOX(barre), fin);
	gtk_box_append(GTK_BOX(hote), pile);
	gtk_box_append(GTK_BOX(hote), barre);

	/* GTK 4 montre par défaut, mais la pile doit avoir choisi une étape. */
	if (etapes > 0) gtk_stack_set_visible_child_name(GTK_STACK(pile), "e0");

	widget_visibility_list_add(hote, attr);
	return hote;
}

/* Export : l'index de l'étape courante, comme <stack>. */
gchar *widget_wizard_envvar_construct(GtkWidget *widget)
{
	if (!widget) return g_strdup("0");
	return g_strdup_printf("%d",
		GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "sermo_etape")));
}
gchar *widget_wizard_envvar_all_construct(variable *var)
{
	if (!var || !var->Widget) return NULL;
	return widget_wizard_envvar_construct(var->Widget);
}
void widget_wizard_clear(variable *var)          { (void) var; }
void widget_wizard_refresh(variable *var)        { (void) var; }
void widget_wizard_fileselect(variable *var, const char *name, const char *value)
{	(void) var; (void) name; (void) value; }
void widget_wizard_removeselected(variable *var) { (void) var; }
void widget_wizard_save(variable *var)           { (void) var; }
