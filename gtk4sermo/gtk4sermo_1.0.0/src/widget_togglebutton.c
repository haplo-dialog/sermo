/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_togglebutton.c — GtkToggleButton (GTK4)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <togglebutton> — bouton à bascule persistant (on/off).
 * Exporte "true"/"false" comme GtkSwitch.
 *
 * XML :
 *   <togglebutton>
 *     <label>Activer</label>
 *     <variable>MY_TOGGLE</variable>
 *     <default>true</default>
 *     <action>echo $MY_TOGGLE</action>
 *   </togglebutton>
 *
 * GtkToggleButton existe toujours en GTK4 sans modification.
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
#include "widget_togglebutton.h"

static void widget_togglebutton_input_by_command(variable *var, char *command);
static void widget_togglebutton_input_by_file(variable *var, char *filename);

void widget_togglebutton_clear(variable *var)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(var->Widget), FALSE);
}

GtkWidget *widget_togglebutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget *widget;
    gchar     *label = NULL;
    GList     *element = NULL;

    if (Attr)
        label = attributeset_get_first(&element, Attr, ATTR_LABEL);

    if (label && *label)
        widget = gtk_toggle_button_new_with_label(label);
    else
        widget = gtk_toggle_button_new();

    return widget;
}

gchar *widget_togglebutton_envvar_all_construct(variable *var)
{
    return NULL;
}

gchar *widget_togglebutton_envvar_construct(GtkWidget *widget)
{
    return g_strdup(
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ? "true" : "false");
}

void widget_togglebutton_fileselect(variable *var, const char *name, const char *value)
{
    g_warning("%s(): Fileselect not implemented.", __func__);
}

void widget_togglebutton_refresh(variable *var)
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
            widget_togglebutton_input_by_command(var, act + 8);
        if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
            if (!initialised)
                widget_file_monitor_try_create(var, act + 5);
            widget_togglebutton_input_by_file(var, act + 5);
        }
        act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
    }

    if (!initialised) {
        if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
            act = attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT);
            gboolean active = (strcasecmp(act, "true") == 0 ||
                               strcasecmp(act, "yes")  == 0 ||
                               atoi(act) == 1);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(var->Widget), active);
        }
        if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
            gtk_widget_set_sensitive(var->Widget, FALSE);

        g_signal_connect(G_OBJECT(var->Widget), "toggled",
            G_CALLBACK(on_any_widget_toggled_event), (gpointer)var->Attributes);
    }
}

void widget_togglebutton_removeselected(variable *var)
{
    g_warning("%s(): Not implemented.", __func__);
}

void widget_togglebutton_save(variable *var)
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
        fprintf(outfile, "%s\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(var->Widget))
            ? "true" : "false");
        fclose(outfile);
    }
}

static void widget_togglebutton_input_by_command(variable *var, char *command)
{
    FILE  *infile;
    gchar  line[512];
    if ((infile = widget_opencommand(command))) {
        if (fgets(line, sizeof(line), infile)) {
            gboolean a = (strcasecmp(line, "true") == 0 ||
                          strcasecmp(line, "yes")  == 0 || atoi(line) == 1);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(var->Widget), a);
        }
        fclose(infile);
    }
}

static void widget_togglebutton_input_by_file(variable *var, char *filename)
{
    FILE  *infile;
    gchar  line[512];
    if ((infile = fopen(filename, "r"))) {
        if (fgets(line, sizeof(line), infile)) {
            gboolean a = (strcasecmp(line, "true") == 0 ||
                          strcasecmp(line, "yes")  == 0 || atoi(line) == 1);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(var->Widget), a);
        }
        fclose(infile);
    }
}
