/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_stack.c — GtkStack (GTK4-native)
 * gtk4sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkStack : conteneur multi-pages avec transition animée.
 * Alternative moderne à GtkNotebook (sans onglets visibles).
 * Combiné avec GtkStackSwitcher pour les sélecteurs.
 *
 * XML :
 *   <stack transition="slide-left-right">
 *     <variable>MY_STACK</variable>
 *     <vbox name="page1" title="Page 1">...</vbox>
 *     <vbox name="page2" title="Page 2">...</vbox>
 *   </stack>
 *
 * Variable exportée : nom de la page visible.
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
#include "widget_stack.h"

void widget_stack_clear(variable *var) {}

GtkWidget *widget_stack_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget                  *widget;
    GtkStackTransitionType      trans = GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT;
    gchar                      *v;

    if (attr) {
        if ((v = get_tag_attribute(attr, "transition"))) {
            if      (strcasecmp(v, "none")              == 0) trans = GTK_STACK_TRANSITION_TYPE_NONE;
            else if (strcasecmp(v, "crossfade")         == 0) trans = GTK_STACK_TRANSITION_TYPE_CROSSFADE;
            else if (strcasecmp(v, "slide-right")       == 0) trans = GTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT;
            else if (strcasecmp(v, "slide-left")        == 0) trans = GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT;
            else if (strcasecmp(v, "slide-up")          == 0) trans = GTK_STACK_TRANSITION_TYPE_SLIDE_UP;
            else if (strcasecmp(v, "slide-down")        == 0) trans = GTK_STACK_TRANSITION_TYPE_SLIDE_DOWN;
            else if (strcasecmp(v, "slide-left-right")  == 0) trans = GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT;
            else if (strcasecmp(v, "slide-up-down")     == 0) trans = GTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN;
        }
    }

    widget = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(widget), trans);

    /* Pop les enfants et les ajouter comme pages */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; i++) {
        if (s.widgets[i]) {
            gchar *name = g_strdup_printf("page%d", i);
            gchar *title = g_strdup_printf("Page %d", i + 1);
            gtk_stack_add_titled(GTK_STACK(widget),
                                 GTK_WIDGET(s.widgets[i]),
                                 name, title);
            g_free(name);
            g_free(title);
        }
    }

    return widget;
}

gchar *widget_stack_envvar_all_construct(variable *var) { return NULL; }

gchar *widget_stack_envvar_construct(GtkWidget *widget)
{
    GtkWidget *child = gtk_stack_get_visible_child(GTK_STACK(widget));
    if (!child) return g_strdup("");
    const gchar *name = gtk_stack_get_visible_child_name(GTK_STACK(widget));
    return g_strdup(name ? name : "");
}

void widget_stack_fileselect(variable *var, const char *name, const char *value)
{
    g_warning("%s(): Not implemented.", __func__);
}

void widget_stack_refresh(variable *var)
{
    GList  *element;
    gchar  *act;
    gint    initialised = FALSE;

    if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
        initialised = GPOINTER_TO_INT(
            g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

    if (!initialised) {
        if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
            act = attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT);
            if (act && *act)
                gtk_stack_set_visible_child_name(GTK_STACK(var->Widget), act);
        }
    }
}

void widget_stack_removeselected(variable *var) {}
void widget_stack_save(variable *var) {}
