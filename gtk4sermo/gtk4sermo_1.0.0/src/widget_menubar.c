/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_menubar.c — Barre de menus GTK4 via GtkPopoverMenuBar + GMenuModel
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkMenuBar a été supprimé en GTK4. L'API officielle de remplacement est :
 *   GMenuModel (structure de données du menu)
 *   GtkPopoverMenuBar (widget de barre de menus, ancré sur la fenêtre)
 *
 * Stratégie :
 *   - widget_menubar_create() construit un GMenu racine
 *   - Les <menuitem> enfants sont ajoutés via widget_menubar_add_item()
 *   - La GtkPopoverMenuBar est créée à partir du GMenu
 *   - Les actions GSimpleAction sont enregistrées dans un GSimpleActionGroup
 *     attaché à la fenêtre (gtk_widget_insert_action_group)
 *
 * Sécurité :
 *   - Noms d'actions sanitisés (remplace les espaces et car. spéciaux)
 *   - Les callbacks d'action passent par safe_system()
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
#include "signals.h"
#include "safe_exec.h"
#include "tag_attributes.h"
#include "stack.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ─── Données associées au widget menubar ───────────────────────────────── */
typedef struct {
    GtkWidget          *popover_bar;  /* GtkPopoverMenuBar affiché */
    GMenu              *root_menu;    /* GMenu racine */
    GSimpleActionGroup *actions;      /* Groupe d'actions GSimpleAction */
    int                 n_items;      /* Nombre d'actions enregistrées */
} MenuBarData;

/* ─── Construit la barre à partir des attributs XML ─────────────────────── */
GtkWidget *widget_menubar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    /* La grammaire garantit qu'une <menubar> contient au moins un <menu> ;
     * ceux-ci ont été empilés avant elle, chacun portant son GMenuModel et son
     * groupe d'actions. L'ancienne version ignorait ses enfants et lisait ses
     * propres attributs, une forme XML que gtkdialog n'a jamais eue : la barre
     * sortait vide, et les <menuitem> empilés en popovers faisaient planter le
     * programme. */
    stackelement        s;
    MenuBarData        *md   = g_new0(MenuBarData, 1);
    GtkWidget          *bar;
    gint                n;

    (void)attr; (void)Type; (void)Attr;

    md->root_menu = g_menu_new();
    md->actions   = g_simple_action_group_new();
    md->n_items   = 0;

    s = pop();

    /* La pile est déjà dans l'ordre du document pour les menus. */
    for (n = 0; n < s.nwidgets; n++) {
        GtkWidget          *enfant = s.widgets[n];
        GMenu              *sm;
        const gchar        *sl;
        GSimpleActionGroup *sa;

        if (!enfant || !G_IS_OBJECT(enfant)) continue;

        sm = g_object_get_data(G_OBJECT(enfant), "_menu_model");
        sl = g_object_get_data(G_OBJECT(enfant), "_menu_label");
        sa = g_object_get_data(G_OBJECT(enfant), "_menu_actions");
        if (!sm) continue;

        g_menu_append_submenu(md->root_menu, (sl && *sl) ? sl : "Menu",
                              G_MENU_MODEL(sm));
        md->n_items++;

        /* Les actions de chaque menu rejoignent le groupe unique de la barre :
         * les GMenuItem les désignent toutes par le préfixe « menu. ». */
        if (sa) {
            gchar **noms = g_action_group_list_actions(G_ACTION_GROUP(sa));
            for (gchar **q = noms; q && *q; q++) {
                GAction *act = g_action_map_lookup_action(G_ACTION_MAP(sa), *q);
                if (act) g_action_map_add_action(G_ACTION_MAP(md->actions), act);
            }
            g_strfreev(noms);
        }
    }

    bar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(md->root_menu));

    /* Le groupe est installé sur la barre elle-même : GTK remonte l'arbre pour
     * résoudre « menu.xxx », donc tous ses éléments le trouvent. */
    gtk_widget_insert_action_group(bar, "menu", G_ACTION_GROUP(md->actions));

    g_object_set_data_full(G_OBJECT(bar), "menubar_data", md,
                           (GDestroyNotify)g_free);
    g_object_set_data(G_OBJECT(bar), "action_group", md->actions);

    gtk_widget_set_hexpand(bar, TRUE);
    gtk_widget_set_visible(bar, TRUE);
    return bar;
}

/* ── widget_menubar_insert_action_group :
 *    À appeler après que le widget est ajouté à une fenêtre.
 *    Enregistre le GSimpleActionGroup sur la fenêtre. */
void widget_menubar_insert_action_group(GtkWidget *menubar, GtkWidget *window)
{
    if (!menubar || !window) return;
    GSimpleActionGroup *ag = g_object_get_data(G_OBJECT(menubar), "action_group");
    if (ag)
        gtk_widget_insert_action_group(window, "menubar",
                                       G_ACTION_GROUP(ag));
}

gchar *widget_menubar_envvar_construct(GtkWidget *widget)       { return g_strdup(""); }
gchar *widget_menubar_envvar_all_construct(variable *var)       { return NULL; }
void   widget_menubar_clear(variable *var)                      {}
void   widget_menubar_refresh(variable *var)                    {}
void   widget_menubar_fileselect(variable *var, const gchar *n, const gchar *v) {}
void   widget_menubar_removeselected(variable *var)             {}
void   widget_menubar_save(variable *var)                       {}
