/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_overlay.c — GtkOverlay (GTK4-native)
 * gtk4sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkOverlay : superpose des widgets par-dessus un enfant principal.
 * Utile pour badges, notifications flottantes, overlays HUD.
 *
 * XML :
 *   <overlay>
 *     <variable>MY_OVERLAY</variable>
 *     <image>main_content.png</image>     <!-- enfant principal (pop[0]) -->
 *     <label halign="end" valign="start"> <!-- overlays (pop[1..n]) -->
 *       Texte flottant
 *     </label>
 *   </overlay>
 *
 * Variable exportée : "" (overlay est un conteneur).
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
#include "stack.h"
#include "tag_attributes.h"
#include "widget_overlay.h"

void widget_overlay_clear(variable *var) {}

GtkWidget *widget_overlay_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget *widget = gtk_overlay_new();

    /* Pop les enfants : le premier est le widget principal,
     * les suivants sont des overlays */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; i++) {
        if (!s.widgets[i]) continue;
        if (i == 0) {
            gtk_overlay_set_child(GTK_OVERLAY(widget), GTK_WIDGET(s.widgets[0]));
        } else {
            gtk_overlay_add_overlay(GTK_OVERLAY(widget), GTK_WIDGET(s.widgets[i]));
        }
    }

    return widget;
}

gchar *widget_overlay_envvar_all_construct(variable *var) { return NULL; }
gchar *widget_overlay_envvar_construct(GtkWidget *widget) { return g_strdup(""); }
void   widget_overlay_fileselect(variable *var, const char *name, const char *value) {}
void   widget_overlay_refresh(variable *var) {}
void   widget_overlay_removeselected(variable *var) {}
void   widget_overlay_save(variable *var) {}
