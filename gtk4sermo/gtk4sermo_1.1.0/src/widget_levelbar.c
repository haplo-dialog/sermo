/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_levelbar.c — GtkLevelBar (GTK4-native)
 * gtk4sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkLevelBar affiche un niveau de remplissage (0.0–1.0 par défaut).
 * Différent de GtkProgressBar : supporte des paliers colorés (offsets).
 *
 * XML :
 *   <levelbar min="0" max="1" value="0.6" inverted="false">
 *     <variable>MY_LEVEL</variable>
 *   </levelbar>
 *
 * Variable exportée : valeur décimale entre min et max.
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
#include "tag_attributes.h"
#include "widget_levelbar.h"

static void widget_levelbar_input_by_command(variable *var, char *command);
static void widget_levelbar_input_by_file(variable *var, char *filename);

void widget_levelbar_clear(variable *var)
{
    gtk_level_bar_set_value(GTK_LEVEL_BAR(var->Widget), 0.0);
}

GtkWidget *widget_levelbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget *widget;
    gdouble    min = 0.0, max = 1.0, value = 0.0;
    gboolean   inverted = FALSE;
    gchar     *v;

    if (attr) {
        if ((v = get_tag_attribute(attr, "min")))   min   = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "max")))   max   = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "value"))) value = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "inverted")) &&
            (strcasecmp(v, "true") == 0 || strcmp(v, "1") == 0))
            inverted = TRUE;
    }

    widget = gtk_level_bar_new_for_interval(min, max);
    gtk_level_bar_set_value(GTK_LEVEL_BAR(widget), value);
    gtk_level_bar_set_inverted(GTK_LEVEL_BAR(widget), inverted);

    /* Paliers standard GTK4 */
    gtk_level_bar_add_offset_value(GTK_LEVEL_BAR(widget),
                                   GTK_LEVEL_BAR_OFFSET_LOW,   0.25 * (max - min) + min);
    gtk_level_bar_add_offset_value(GTK_LEVEL_BAR(widget),
                                   GTK_LEVEL_BAR_OFFSET_HIGH,  0.75 * (max - min) + min);
    gtk_level_bar_add_offset_value(GTK_LEVEL_BAR(widget),
                                   GTK_LEVEL_BAR_OFFSET_FULL,  max);

    return widget;
}

gchar *widget_levelbar_envvar_all_construct(variable *var) { return NULL; }

gchar *widget_levelbar_envvar_construct(GtkWidget *widget)
{
    return g_strdup_printf("%g",
        gtk_level_bar_get_value(GTK_LEVEL_BAR(widget)));
}

void widget_levelbar_fileselect(variable *var, const char *name, const char *value)
{
    g_warning("%s(): Not implemented.", __func__);
}

void widget_levelbar_refresh(variable *var)
{
    GList  *element;
    gchar  *act;
    gint    initialised = FALSE;

    if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
        initialised = GPOINTER_TO_INT(
            g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

    act = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
    while (act) {
        if (input_is_shell_command(act))
            widget_levelbar_input_by_command(var, act + 8);
        if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
            if (!initialised)
                widget_file_monitor_try_create(var, act + 5);
            widget_levelbar_input_by_file(var, act + 5);
        }
        act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
    }

    if (!initialised) {
        if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
            act = attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT);
            if (act) gtk_level_bar_set_value(GTK_LEVEL_BAR(var->Widget), g_ascii_strtod(act, NULL));
        }
        if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
            gtk_widget_set_sensitive(var->Widget, FALSE);
    }
}

void widget_levelbar_removeselected(variable *var) {}

void widget_levelbar_save(variable *var)
{
    FILE  *outfile;
    GList *element;
    gchar *act, *filename = NULL;

    act = attributeset_get_first(&element, var->Attributes, ATTR_OUTPUT);
    while (act) {
        if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
            filename = act + 5; break;
        }
        act = attributeset_get_next(&element, var->Attributes, ATTR_OUTPUT);
    }
    if (filename && (outfile = fopen(filename, "w"))) {
        fprintf(outfile, "%g\n",
            gtk_level_bar_get_value(GTK_LEVEL_BAR(var->Widget)));
        fclose(outfile);
    }
}

static void widget_levelbar_input_by_command(variable *var, char *command)
{
    FILE *infile;
    gchar line[256];
    if ((infile = widget_opencommand(command))) {
        if (fgets(line, sizeof(line), infile))
            gtk_level_bar_set_value(GTK_LEVEL_BAR(var->Widget), g_ascii_strtod(line, NULL));
        fclose(infile);
    }
}

static void widget_levelbar_input_by_file(variable *var, char *filename)
{
    FILE *infile;
    gchar line[256];
    if ((infile = fopen(filename, "r"))) {
        if (fgets(line, sizeof(line), infile))
            gtk_level_bar_set_value(GTK_LEVEL_BAR(var->Widget), g_ascii_strtod(line, NULL));
        fclose(infile);
    }
}
