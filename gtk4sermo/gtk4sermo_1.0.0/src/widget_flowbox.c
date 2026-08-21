/*
 * widget_flowbox.c — GtkFlowBox (GTK4-native)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkFlowBox : grille flexible qui redistribue ses enfants selon
 * la largeur disponible (comme un flux de texte).
 * Idéal pour galeries d'images, grilles de boutons adaptatifs.
 *
 * XML :
 *   <flowbox min-children-per-line="2" max-children-per-line="8"
 *            column-spacing="6" row-spacing="6" selection-mode="none">
 *     <variable>MY_FLOW</variable>
 *     <button>...</button>
 *     <button>...</button>
 *   </flowbox>
 *
 * Variable exportée : index de l'enfant sélectionné (ou "").
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
#include "widget_flowbox.h"

void widget_flowbox_clear(variable *var)
{
    /* Supprimer tous les enfants */
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(var->Widget)) != NULL)
        gtk_flow_box_remove(GTK_FLOW_BOX(var->Widget), child);
}

GtkWidget *widget_flowbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget          *widget;
    guint               min_per_line = 1, max_per_line = 4;
    gint                col_spacing = 6, row_spacing = 6;
    GtkSelectionMode    sel_mode = GTK_SELECTION_NONE;
    gchar              *v;

    if (attr) {
        if ((v = get_tag_attribute(attr, "min-children-per-line"))) min_per_line = (guint)atoi(v);
        if ((v = get_tag_attribute(attr, "max-children-per-line"))) max_per_line = (guint)atoi(v);
        if ((v = get_tag_attribute(attr, "column-spacing")))        col_spacing  = atoi(v);
        if ((v = get_tag_attribute(attr, "row-spacing")))           row_spacing  = atoi(v);
        if ((v = get_tag_attribute(attr, "selection-mode"))) {
            if      (strcasecmp(v, "single")   == 0) sel_mode = GTK_SELECTION_SINGLE;
            else if (strcasecmp(v, "browse")   == 0) sel_mode = GTK_SELECTION_BROWSE;
            else if (strcasecmp(v, "multiple") == 0) sel_mode = GTK_SELECTION_MULTIPLE;
        }
    }

    widget = gtk_flow_box_new();
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(widget), min_per_line);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(widget), max_per_line);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(widget), (guint)col_spacing);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(widget), (guint)row_spacing);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(widget), sel_mode);

    /* Pop les enfants de la pile */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; i++) {
        if (s.widgets[i])
            gtk_flow_box_append(GTK_FLOW_BOX(widget), GTK_WIDGET(s.widgets[i]));
    }

    return widget;
}

gchar *widget_flowbox_envvar_all_construct(variable *var) { return NULL; }

gchar *widget_flowbox_envvar_construct(GtkWidget *widget)
{
    GList *selected = gtk_flow_box_get_selected_children(GTK_FLOW_BOX(widget));
    if (!selected) return g_strdup("");
    GtkFlowBoxChild *child = GTK_FLOW_BOX_CHILD(selected->data);
    gchar *result = g_strdup_printf("%d", gtk_flow_box_child_get_index(child));
    g_list_free(selected);
    return result;
}

void widget_flowbox_fileselect(variable *var, const char *name, const char *value)
{
    g_warning("%s(): Not implemented.", __func__);
}

void widget_flowbox_refresh(variable *var)
{
    gint initialised = FALSE;
    if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
        initialised = GPOINTER_TO_INT(
            g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

    if (!initialised) {
        if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
            gtk_widget_set_sensitive(var->Widget, FALSE);
    }
}

void widget_flowbox_removeselected(variable *var) {}
void widget_flowbox_save(variable *var) {}
