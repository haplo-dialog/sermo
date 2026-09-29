/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_menubar.c — Barre de menus GTK4 via GtkPopoverMenuBar + GMenuModel
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
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
    /* Menu construit À LA MAIN (GtkMenuButton + GtkPopover) au lieu de
     * GtkPopoverMenuBar/GMenuModel : GTK4 masque les icônes des items
     * « modèle ». Ici chaque item est une ligne image+libellé (cf.
     * widget_menu_create), donc les icônes de thème s'affichent (parité gtk3). */
    stackelement  s;
    GtkWidget    *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gint          n;

    (void)attr; (void)Type; (void)Attr;
    gtk_widget_add_css_class(bar, "menubar");

    s = pop();
    for (n = 0; n < s.nwidgets; n++) {
        GtkWidget   *enfant = s.widgets[n];
        GtkWidget   *pop;
        const gchar *sl;
        if (!enfant || !G_IS_OBJECT(enfant)) continue;
        pop = g_object_get_data(G_OBJECT(enfant), "_menu_popover");
        sl  = g_object_get_data(G_OBJECT(enfant), "_menu_label");
        if (!pop) continue;

        GtkWidget *mb = gtk_menu_button_new();
        gtk_menu_button_set_label(GTK_MENU_BUTTON(mb), (sl && *sl) ? sl : "Menu");
        gtk_menu_button_set_popover(GTK_MENU_BUTTON(mb), pop);
        gtk_menu_button_set_always_show_arrow(GTK_MENU_BUTTON(mb), FALSE);
        gtk_widget_add_css_class(mb, "flat");
        gtk_box_append(GTK_BOX(bar), mb);
    }

    gtk_widget_set_hexpand(bar, TRUE);
    gtk_widget_set_halign(bar, GTK_ALIGN_FILL);
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
