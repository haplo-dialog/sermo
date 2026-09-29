/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_stackpages.c — N pages, une seule visible (GTK 3)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <stack> est <notebook> SANS les onglets : le script décide de la page.
 * GtkStack fait exactement ça, et GtkStackSwitcher fournit la rangée de
 * boutons quand switcher="true". Ce port est l'ÉTALON : il fixe la valeur exportée.
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
#include "widget_stackpages.h"
#include <string.h>
#include <stdlib.h>

/* ⚠️ GtkStack refuse de rendre visible un enfant qui n'a pas encore été
 * MONTRÉ : dans sermo, les widgets ne le sont qu'à la fin (widget_show_all).
 * gtk_stack_set_visible_child_name() posé à la création ne prend donc PAS —
 * get_visible_child_name() rendait NULL, et la page demandée était ignorée en
 * silence (mesuré : page="1" restait sur la page 0). On garde la page voulue
 * sur l'objet et on l'applique au premier « map », quand l'arbre est réel. */
/* L'utilisateur a changé de page (switcher) : on met à jour l'index retenu,
 * qui est la SEULE source de vérité pour l'export. Lire
 * gtk_stack_get_visible_child_name() au moment de l'export donnait des
 * réponses différentes selon que la fenêtre avait été affichée ou non — donc
 * selon qu'on tourne avec ou sans écran. */
static void stack_page_changee(GObject *pile, GParamSpec *p, gpointer inutile)
{
	const gchar *nom = gtk_stack_get_visible_child_name(GTK_STACK(pile));

	(void) p; (void) inutile;
	/* ⚠️ Tant que la page demandée n'a pas été POSÉE (voir stack_poser_page),
	 * GTK émet ce signal tout seul en choisissant le premier enfant : l'écouter
	 * trop tôt écrasait page="1" par 0. On ne suit l'utilisateur qu'après. */
	if (!g_object_get_data(pile, "sermo_pose")) return;
	if (nom && nom[0] == 'p')
		g_object_set_data(pile, "sermo_page", GINT_TO_POINTER(atoi(nom + 1)));
}

static gboolean stack_poser_page(gpointer donnee)
{
	GtkWidget *pile = GTK_WIDGET(donnee);
	gint       page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(pile), "sermo_page"));
	gchar      nom[16];

	g_snprintf(nom, sizeof(nom), "p%d", page);
	gtk_stack_set_visible_child_name(GTK_STACK(pile), nom);
	g_object_set_data(G_OBJECT(pile), "sermo_pose", GINT_TO_POINTER(1));
	return G_SOURCE_REMOVE;   /* une seule fois */
}

GtkWidget *widget_stackpages_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget    *pile, *enveloppe = NULL;
	stackelement  s;
	gchar        *value;
	gint          n, page = 0, avec_switcher = 0;

	(void) Attr; (void) Type;

	if (attr && (value = get_tag_attribute(attr, "page")))
		page = atoi(value);
	if (attr && (value = get_tag_attribute(attr, "switcher")))
		avec_switcher = (strcasecmp(value, "true") == 0 ||
		                 strcasecmp(value, "yes") == 0 || atoi(value) == 1);

	pile = gtk_stack_new();

	s = pop();
	for (n = 0; n < s.nwidgets; ++n) {
		gchar nom[16], titre[16];
		if (!s.widgets[n]) continue;
		/* Un nom par page : c'est lui que GtkStack manipule, et le titre est ce
		 * que le switcher affiche. Sans titre, le switcher reste vide. */
		g_snprintf(nom, sizeof(nom), "p%d", n);
		g_snprintf(titre, sizeof(titre), "%d", n + 1);
		gtk_stack_add_titled(GTK_STACK(pile), s.widgets[n], nom, titre);
	}
	if (page < 0) page = 0;
	if (page >= s.nwidgets && s.nwidgets > 0) page = s.nwidgets - 1;
	g_object_set_data(G_OBJECT(pile), "sermo_page", GINT_TO_POINTER(page));
	/* ⚠️ Ni à la création, ni au « map » : GtkStack ne retient une page que
	 * lorsque l'enfant est réellement affiché, ce qui n'arrive qu'après le
	 * widget_show_all() du cœur — et le switcher, en s'attachant, repose la
	 * page 0. On passe donc par la boucle principale : g_idle_add s'exécute
	 * quand tout l'arbre est en place, sur le thread GTK (⛔ jamais depuis un
	 * thread secondaire). */
	g_idle_add(stack_poser_page, pile);
	g_signal_connect(pile, "notify::visible-child-name",
	                 G_CALLBACK(stack_page_changee), NULL);

	if (avec_switcher) {
		/* La rangée de boutons vit AU-DESSUS de la pile, dans une boîte : le
		 * tout se comporte comme un seul widget pour le reste du dialogue. */
		GtkWidget *sw = gtk_stack_switcher_new();
		gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(sw), GTK_STACK(pile));
		enveloppe = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
		gtk_box_pack_start(GTK_BOX(enveloppe), sw, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(enveloppe), pile, TRUE, TRUE, 0);
		g_object_set_data(G_OBJECT(enveloppe), "sermo_pile", pile);
	}

	widget_visibility_list_add(enveloppe ? enveloppe : pile, attr);
	return enveloppe ? enveloppe : pile;
}

/* Export : l'index de la page visible, comme <notebook>. */
gchar *widget_stackpages_envvar_construct(GtkWidget *widget)
{
	GtkWidget   *pile = widget;
	const gchar *nom;

	if (!widget) return g_strdup("0");
	/* Avec un switcher, le widget rendu est l'ENVELOPPE : la pile y est
	 * accrochée. Sans, c'est la pile elle-même. */
	if (g_object_get_data(G_OBJECT(widget), "sermo_pile"))
		pile = (GtkWidget *) g_object_get_data(G_OBJECT(widget), "sermo_pile");
	if (!GTK_IS_STACK(pile)) return g_strdup("0");

	(void) nom;
	/* L'index retenu, tenu à jour par stack_page_changee() : même réponse avec
	 * ou sans écran. */
	return g_strdup_printf("%d",
		GPOINTER_TO_INT(g_object_get_data(G_OBJECT(pile), "sermo_page")));
}
gchar *widget_stackpages_envvar_all_construct(variable *var)
{
	if (!var || !var->Widget) return NULL;
	return widget_stackpages_envvar_construct(var->Widget);
}
void widget_stackpages_clear(variable *var)          { (void) var; }
void widget_stackpages_refresh(variable *var)        { (void) var; }
void widget_stackpages_fileselect(variable *var, const char *name, const char *value)
{	(void) var; (void) name; (void) value; }
void widget_stackpages_removeselected(variable *var) { (void) var; }
void widget_stackpages_save(variable *var)           { (void) var; }
