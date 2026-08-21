/*
 * widget_menuitem.c — Éléments de menu GTK4 via GMenuItem + GSimpleAction
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Dans le modèle GTK4 :
 *   - <menuitem> seul (sans <menubar> parent) → GtkPopoverMenu flottant
 *   - <menu> contenant des <menuitem> → GMenu + GtkPopoverMenu
 *
 * Sécurité :
 *   - Actions enregistrées dans un GSimpleActionGroup local
 *   - Callbacks via safe_system()
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

/* Wrapper GClosureNotify (evite -Wcast-function-type sur g_free) */
static void _gtkd_closure_g_free(gpointer data, GClosure *closure) {
	(void)closure;
	g_free(data);
}

/* ─── Sanitise un nom d'action GLib ──────────────────────────────────────── */
static gchar *_menuitem_action_name(const gchar *label, int idx)
{
    if (!label || !*label)
        return g_strdup_printf("mi_%d", idx);
    gchar *name = g_strdup_printf("mi_%d_%s", idx, label);
    for (gchar *p = name; *p; p++) {
        if (!g_ascii_isalnum(*p) && *p != '_' && *p != '-')
            *p = '_';
        else
            *p = g_ascii_tolower(*p);
    }
    return name;
}

/* ─── Callback action ────────────────────────────────────────────────────── */
static void _menuitem_activate(GSimpleAction *a, GVariant *p, gpointer data)
{
    (void)a; (void)p;
    const gchar *cmd = (const gchar *)data;
    if (cmd && *cmd) safe_system(cmd);
}

/* ─── widget_menuitem_create — retourne un GtkPopoverMenu autonome ────────── */
GtkWidget *widget_menuitem_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GMenu              *menu    = g_menu_new();
    GSimpleActionGroup *actions = g_simple_action_group_new();
    int                 idx     = 0;

    if (Attr) {
        GList *il = NULL;
        gchar *item = attributeset_get_first(&il, Attr, ATTR_ITEM);
        while (item) {
            if (*item) {
                gchar *sep = strchr(item, '|');
                gchar *lbl = item, *cmd_str = NULL;
                if (sep) { *sep = '\0'; cmd_str = sep + 1; while (*cmd_str == ' ') cmd_str++; }
                size_t l = strlen(lbl);
                while (l > 0 && lbl[l-1] == ' ') lbl[--l] = '\0';

                gchar *an   = _menuitem_action_name(lbl, idx);
                gchar *full = g_strdup_printf("menuitem.%s", an);

                GSimpleAction *action = g_simple_action_new(an, NULL);
                g_signal_connect_data(action, "activate",
                    G_CALLBACK(_menuitem_activate),
                    cmd_str ? g_strdup(cmd_str) : g_strdup(""),
                    _gtkd_closure_g_free, 0);
                g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));
                g_object_unref(action);

                g_menu_append(menu, lbl, full);
                g_free(an); g_free(full);
                idx++;
            }
            item = attributeset_get_next(&il, Attr, ATTR_ITEM);
        }
    }

    /* GtkPopoverMenu depuis le GMenuModel */
    GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);

    /* Le groupe d'actions sera attaché à la fenêtre parente via realize */
    g_object_set_data(G_OBJECT(popover), "action_group", actions);
    g_object_set_data_full(G_OBJECT(popover), "_menu_model", menu,
                           g_object_unref);

    gtk_widget_set_visible(popover, TRUE);
    return popover;
}

/* widget_menu_create — conteneur de menuitems (alias) */
GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    return widget_menuitem_create(Attr, attr, Type);
}

gchar *widget_menuitem_envvar_construct(GtkWidget *w)               { return g_strdup(""); }
gchar *widget_menuitem_envvar_all_construct(variable *var)          { return NULL; }
void   widget_menuitem_clear(variable *var)                         {}
void   widget_menuitem_refresh(variable *var)                       {}
void   widget_menuitem_fileselect(variable *var, const gchar *n, const gchar *v) {}
void   widget_menuitem_removeselected(variable *var)                {}
void   widget_menuitem_save(variable *var)                          {}
