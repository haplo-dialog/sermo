/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_revealer.c — GtkRevealer (GTK4-native)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkRevealer affiche/cache son enfant avec une animation.
 * Utile pour panneaux dépliants, alertes animées, etc.
 *
 * XML :
 *   <revealer transition="slide-down" duration="300" reveal="true">
 *     <variable>MY_REVEALER</variable>
 *     <vbox>...</vbox>
 *   </revealer>
 *
 * Transitions : none | crossfade | slide-right | slide-left |
 *               slide-up | slide-down (défaut: slide-down)
 * Variable exportée : "true"/"false" (état reveal).
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
#include "widget_revealer.h"

void widget_revealer_clear(variable *var)
{
    gtk_revealer_set_reveal_child(GTK_REVEALER(var->Widget), FALSE);
}

GtkWidget *widget_revealer_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget              *widget;
    GtkRevealerTransitionType trans = GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN;
    gint                    duration = 250;
    gboolean                reveal = FALSE;
    gchar                  *v;

    if (attr) {
        if ((v = get_tag_attribute(attr, "transition"))) {
            if      (strcasecmp(v, "none")       == 0) trans = GTK_REVEALER_TRANSITION_TYPE_NONE;
            else if (strcasecmp(v, "crossfade")  == 0) trans = GTK_REVEALER_TRANSITION_TYPE_CROSSFADE;
            else if (strcasecmp(v, "slide-right")== 0) trans = GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT;
            else if (strcasecmp(v, "slide-left") == 0) trans = GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT;
            else if (strcasecmp(v, "slide-up")   == 0) trans = GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP;
            else if (strcasecmp(v, "slide-down") == 0) trans = GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN;
        }
        if ((v = get_tag_attribute(attr, "duration"))) duration = atoi(v);
        if ((v = get_tag_attribute(attr, "reveal")) &&
            (strcasecmp(v, "true") == 0 || strcmp(v, "1") == 0))
            reveal = TRUE;
    }

    widget = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(widget), trans);
    gtk_revealer_set_transition_duration(GTK_REVEALER(widget), (guint)duration);
    gtk_revealer_set_reveal_child(GTK_REVEALER(widget), reveal);

    /* Pop l'enfant de la pile */
    stackelement s = pop();
    if (s.nwidgets > 0 && s.widgets[0])
        gtk_revealer_set_child(GTK_REVEALER(widget), GTK_WIDGET(s.widgets[0]));

    return widget;
}

gchar *widget_revealer_envvar_all_construct(variable *var) { return NULL; }

gchar *widget_revealer_envvar_construct(GtkWidget *widget)
{
    return g_strdup(
        gtk_revealer_get_reveal_child(GTK_REVEALER(widget)) ? "true" : "false");
}

void widget_revealer_fileselect(variable *var, const char *name, const char *value)
{
    g_warning("%s(): Not implemented.", __func__);
}

void widget_revealer_refresh(variable *var)
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
            gboolean rv = (strcasecmp(act, "true") == 0 || atoi(act) == 1);
            gtk_revealer_set_reveal_child(GTK_REVEALER(var->Widget), rv);
        }
    }
}

void widget_revealer_removeselected(variable *var) {}
void widget_revealer_save(variable *var) {}
