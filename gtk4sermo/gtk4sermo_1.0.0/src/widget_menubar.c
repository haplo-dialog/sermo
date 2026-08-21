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

/* ─── Sanitise un label en nom d'action GLib valide ──────────────────────── */
/* Les noms d'actions GLib ne peuvent contenir que [a-z0-9._-] */
static gchar *_action_name(const gchar *label, int index)
{
    if (!label || !*label)
        return g_strdup_printf("action_%d", index);
    gchar *name = g_strdup(label);
    for (gchar *p = name; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-')
            *p = '_';
        else
            *p = tolower((unsigned char)*p);
    }
    return name;
}

/* ─── Callback GSimpleAction → safe_system ───────────────────────────────── */
typedef struct { char *cmd; } ActionData;

static void _action_cb(GSimpleAction *action, GVariant *param, gpointer data)
{
    (void)action; (void)param;
    ActionData *ad = (ActionData *)data;
    if (ad && ad->cmd && *ad->cmd)
        safe_system(ad->cmd);
}

/* ─── Construit la barre à partir des attributs XML ─────────────────────── */
GtkWidget *widget_menubar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    MenuBarData *md = g_new0(MenuBarData, 1);
    md->root_menu = g_menu_new();
    md->actions   = g_simple_action_group_new();
    md->n_items   = 0;

    /* ── Parser les entrées de menu depuis les attributs ─────────────── */
    if (Attr) {
        /* Format XML attendu :
         *   <menubar>
         *     <menuitem>
         *       <label>Fichier</label>
         *       <item>Ouvrir | cmd_ouvrir</item>
         *       <item>Quitter | gtk-main-quit</item>
         *     </menuitem>
         *   </menubar>
         *
         * Chaque top-level label = un sous-menu dans la menubar.
         * Chaque ATTR_ITEM = "Label | commande" ou juste "Label".
         */
        GList *el = NULL;
        gchar *menu_label = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (!menu_label || !*menu_label)
            menu_label = "Menu";

        /* Créer un sous-menu pour ce label */
        GMenu *submenu = g_menu_new();

        GList *il = NULL;
        gchar *item = attributeset_get_first(&il, Attr, ATTR_ITEM);
        while (item) {
            if (*item) {
                /* Séparer "Label | commande" */
                gchar *sep = strchr(item, '|');
                gchar *item_label = item;
                gchar *item_cmd   = NULL;
                if (sep) {
                    *sep = '\0';
                    item_cmd = sep + 1;
                    while (*item_cmd == ' ') item_cmd++;
                }
                /* Trim trailing spaces */
                size_t len = strlen(item_label);
                while (len > 0 && item_label[len-1] == ' ') item_label[--len] = '\0';

                /* Créer l'action */
                gchar *action_name = _action_name(item_label, md->n_items);
                gchar *full_action = g_strdup_printf("menubar.%s", action_name);

                ActionData *ad = g_new0(ActionData, 1);
                ad->cmd = item_cmd ? g_strdup(item_cmd) : g_strdup("");

                GSimpleAction *action = g_simple_action_new(action_name, NULL);
                g_signal_connect_data(action, "activate",
                                      G_CALLBACK(_action_cb), ad,
                                      (GClosureNotify)NULL, 0);
                g_action_map_add_action(G_ACTION_MAP(md->actions),
                                        G_ACTION(action));
                g_object_unref(action);

                /* Ajouter au sous-menu */
                g_menu_append(submenu, item_label, full_action);
                g_free(action_name);
                g_free(full_action);
                md->n_items++;
            }
            item = attributeset_get_next(&il, Attr, ATTR_ITEM);
        }

        g_menu_append_submenu(md->root_menu, menu_label,
                              G_MENU_MODEL(submenu));
        g_object_unref(submenu);
    }

    /* ── Créer la GtkPopoverMenuBar ──────────────────────────────────── */
    GtkWidget *bar = gtk_popover_menu_bar_new_from_model(
        G_MENU_MODEL(md->root_menu));

    /* Attacher le groupe d'actions à la fenêtre racine.
     * Si la fenêtre n'est pas encore disponible, on le fait via realize. */
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
