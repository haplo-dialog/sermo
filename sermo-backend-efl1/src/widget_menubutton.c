/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubutton.c — Bouton qui déroule un menu (EFL/Elementary)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * elm_hoversel EST ce tag : un bouton qui déroule une liste de choix.
 * <menuitem> pose son modèle (EMenuItem) dans les données de l'objet porteur,
 * sous la clé « emi » — même convention que <menubar> sur ce port.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "actions.h"
#include "efl_menu_model.h"
#include "widget_menubutton.h"
#include <stdlib.h>
#include <string.h>

static void hoversel_choisi(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *it = (Elm_Object_Item *) event_info;
    const char *lbl = it ? elm_object_item_text_get(it) : NULL;
    const char *cmd = (const char *) data;

    if (lbl) {
        char *ancien = evas_object_data_del(obj, "sermo_choix");
        free(ancien);
        evas_object_data_set(obj, "sermo_choix", strdup(lbl));
        elm_object_text_set(obj, lbl);
    }
    if (cmd && *cmd) execute_action((GtkWidget *) obj, (char *) cmd, NULL);
}

GtkWidget *widget_menubutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;

    Evas_Object *parent = efl_main_win_get();
    Evas_Object *hs = elm_hoversel_add(parent ? parent
                                              : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
    const char *label = NULL;

    if (Attr) {
        GList *el = NULL;
        gchar *l = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (l && *l) label = l;
    }
    if (!label && attr) { const char *v = get_tag_attribute(attr, "label"); if (v) label = v; }
    elm_object_text_set(hs, label ? label : "Menu");
    elm_hoversel_hover_parent_set(hs, parent);

    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n) {
        Evas_Object *porteur = (Evas_Object *) s.widgets[n];
        if (!porteur) continue;
        EMenuItem *mi = (EMenuItem *) evas_object_data_get(porteur, "emi");
        if (!mi || mi->separator || !mi->label) continue;
        elm_hoversel_item_add(hs, mi->label, NULL, ELM_ICON_NONE,
                              hoversel_choisi, mi->cmd);
    }
    evas_object_show(hs);
    return (GtkWidget *) hs;
}

/* Export : le libellé du dernier élément choisi, vide avant tout choix. */
gchar *widget_menubutton_envvar_construct(GtkWidget *widget)
{
    const char *choix = widget
        ? (const char *) evas_object_data_get((Evas_Object *) widget, "sermo_choix")
        : NULL;
    return g_strdup(choix ? choix : "");
}
gchar *widget_menubutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_menubutton_envvar_construct(var->Widget);
}
void widget_menubutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    free(evas_object_data_del((Evas_Object *) var->Widget, "sermo_choix"));
}
void widget_menubutton_refresh(variable *var)        { (void) var; }
void widget_menubutton_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_menubutton_removeselected(variable *var) { (void) var; }
void widget_menubutton_save(variable *var)           { (void) var; }
