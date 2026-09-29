/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubar.c — Barre de menus EFL via elm_toolbar + elm_menu
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Stratégie :
 *   elm_toolbar horizontal positionné en haut de la fenêtre.
 *   Chaque élément toolbar déclenche un elm_menu au clic.
 *   Les <menuitem> enfants sont ajoutés dynamiquement dans le menu popup.
 *
 *   C'est l'équivalent fonctionnel le plus proche d'une GtkMenuBar sous EFL.
 *   elm_menu est un menu popup contextuel — sa position est fixée sous le
 *   bouton toolbar pour simuler une barre persistante.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "automaton.h"
#include "attributes.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_menubar.h"
#include "stack.h"
#include "efl_menu_model.h"
#include "sermo_icon_theme.h"
Evas_Object *efl_theme_icon_new(Evas_Object *parent, const char *icon, int px);
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>

/* ─── Données associées à un item toolbar ───────────────────────────────── */
typedef struct {
    Evas_Object *win;      /* Fenêtre parente pour elm_menu_add */
    Evas_Object *menu;     /* Menu popup associé à cet item */
} ToolbarItemData;

/* ─── Callback : clic sur un item du menu popup ─────────────────────────── */
extern void execute_action(GtkWidget *widget, const char *command, const char *type);
static void _menu_item_clicked(void *data, Evas_Object *obj, void *event_info)
{
    const char *cmd = (const char *)data;
    if (!cmd || !*cmd) return;
    execute_action((GtkWidget *)obj, cmd, NULL);
}

/* ─── Callback : clic sur un bouton toolbar → affiche le menu popup ─────── */
static void _toolbar_item_selected(void *data, Evas_Object *obj, void *event_info)
{
    ToolbarItemData *tid = (ToolbarItemData *)data;
    if (!tid || !tid->menu) return;

    /* Récupère la position de l'item toolbar pour ancrer le menu sous lui */
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    Evas_Object *item_obj = elm_object_item_widget_get(item);
    Evas_Coord x = 0, y = 0, h = 0;
    if (item_obj) {
        evas_object_geometry_get(item_obj, &x, &y, NULL, &h);
    }
    elm_menu_move(tid->menu, x, y + h);
    evas_object_show(tid->menu);
}

GtkWidget *widget_menubar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    if (!win) {
        /* Fenêtre pas encore créée — retourne un label invisible */
        Evas_Object *lbl = elm_label_add(elm_win_add(NULL, "efl1dialog-tmp", ELM_WIN_BASIC));
        evas_object_hide(lbl);
        return (GtkWidget *)lbl;
    }

    /* ── Créer la barre toolbar horizontale ──────────────────────────────── */
    Evas_Object *toolbar = elm_toolbar_add(win);
    elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_SCROLL);
    elm_toolbar_homogeneous_set(toolbar, EINA_FALSE);
    elm_toolbar_align_set(toolbar, 0.0); /* aligné à gauche */
    elm_toolbar_select_mode_set(toolbar, ELM_OBJECT_SELECT_MODE_NONE);
    evas_object_size_hint_weight_set(toolbar, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(toolbar, EVAS_HINT_FILL, 0.5);

    /* ── Dépiler les <menu> (carriers de l'arbre) et bâtir toolbar + menus ── */
    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; n++) {
        Evas_Object *w = (Evas_Object *)s.widgets[n];
        if (!w) continue;
        EMenu *m = (EMenu *)evas_object_data_get(w, "emenu");
        if (!m) continue;

        Evas_Object *menu = elm_menu_add(win);
        ToolbarItemData *tid = calloc(1, sizeof(ToolbarItemData));
        if (!tid) continue;
        tid->win = win; tid->menu = menu;
        elm_toolbar_item_append(toolbar, NULL, m->label ? m->label : "Menu",
                                _toolbar_item_selected, tid);

        for (int k = 0; k < m->n; k++) {
            EMenuItem *mi = &m->items[k];
            if (mi->separator) { elm_menu_item_separator_add(menu, NULL); continue; }
            char *cmd_copy = (mi->cmd && *mi->cmd) ? strdup(mi->cmd) : NULL;
            /* 3e arg = nom d'icône standard : Elementary/Efreet le résout via le
             * thème d'icônes freedesktop du bureau. Repli : elm_image (part icon). */
            Elm_Object_Item *it = elm_menu_item_add(menu, NULL,
                                     (mi->icon && *mi->icon) ? mi->icon : NULL,
                                     mi->label ? mi->label : "", _menu_item_clicked, cmd_copy);
            if (it && mi->icon && *mi->icon && !elm_object_item_part_content_get(it, "icon")) {
                Evas_Object *icon = efl_theme_icon_new(menu, mi->icon,
                                        sermo_icon_size_px("menu"));
                if (icon) elm_object_item_part_content_set(it, "icon", icon);
            }
        }
    }

    evas_object_show(toolbar);
    return (GtkWidget *)toolbar;
}

gchar *widget_menubar_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_menubar_envvar_all_construct(variable *var) { return NULL; }
void   widget_menubar_clear(variable *var) {}
void   widget_menubar_refresh(variable *var) {}
void   widget_menubar_fileselect(variable *var, const char *n, const char *v) {}
void   widget_menubar_removeselected(variable *var) {}
void   widget_menubar_save(variable *var) {}
