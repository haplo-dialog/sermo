/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubutton.c — Bouton qui déroule un menu (GTK 4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ GTK 4 n'a plus GtkMenuItem ni GtkMenu : <menuitem> y fabrique un
 * GtkButton (widget_menuitem.c de ce port). Le menu est donc un GtkPopover
 * garni d'une boîte de ces boutons — et c'est le rendu attendu en GTK 4, où
 * les menus SONT des popovers.
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
#include "widget_menubutton.h"
#include <string.h>
#include <stdlib.h>

static void menubutton_choisi(GtkButton *item, gpointer bouton)
{
    const gchar *lbl = gtk_button_get_label(item);

    g_object_set_data_full(G_OBJECT(bouton), "sermo_choix",
                           g_strdup(lbl ? lbl : ""), g_free);
    /* Refermer le popover : en GTK 4 il ne se ferme pas tout seul. */
    {
        /* ⚠️ En GTK 4, gtk_menu_button_get_popover() rend un GtkPopover*,
         * pas un GtkWidget* — le shim de ce port est typé strictement. */
        GtkPopover *pop = gtk_menu_button_get_popover(GTK_MENU_BUTTON(bouton));
        if (pop) gtk_popover_popdown(pop);
    }
}

GtkWidget *widget_menubutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget    *bouton, *popover, *boite;
    stackelement  s;
    GList        *element;
    gchar        *label = NULL;
    gint          n;

    (void) Type;

    if (Attr && attributeset_is_avail(Attr, ATTR_LABEL))
        label = attributeset_get_first(&element, Attr, ATTR_LABEL);
    if (!label && attr) label = get_tag_attribute(attr, "label");

    bouton = gtk_menu_button_new();
    if (label && *label) gtk_menu_button_set_label(GTK_MENU_BUTTON(bouton), label);

    boite = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    s = pop();
    for (n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        gtk_box_append(GTK_BOX(boite), s.widgets[n]);
        if (GTK_IS_BUTTON(s.widgets[n]))
            g_signal_connect(s.widgets[n], "clicked",
                             G_CALLBACK(menubutton_choisi), bouton);
    }
    popover = gtk_popover_new();
    gtk_popover_set_child(GTK_POPOVER(popover), boite);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(bouton), popover);

    return bouton;
}

/* Export : le libellé du dernier élément choisi, vide avant tout choix. */
gchar *widget_menubutton_envvar_construct(GtkWidget *widget)
{
    const gchar *choix;

    if (!widget) return g_strdup("");
    choix = (const gchar *) g_object_get_data(G_OBJECT(widget), "sermo_choix");
    return g_strdup(choix ? choix : "");
}
gchar *widget_menubutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_menubutton_envvar_construct(var->Widget);
}
void widget_menubutton_clear(variable *var)
{
    if (var && var->Widget)
        g_object_set_data(G_OBJECT(var->Widget), "sermo_choix", NULL);
}
void widget_menubutton_refresh(variable *var)        { (void) var; }
void widget_menubutton_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_menubutton_removeselected(variable *var) { (void) var; }
void widget_menubutton_save(variable *var)           { (void) var; }
