/*
 * widget_vscale.c — GtkScale vertical (GTK4)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <vscale> — curseur vertical. Même fonctionnement que <hscale>
 * mais avec GTK_ORIENTATION_VERTICAL.
 *
 * XML :
 *   <vscale range-min="0" range-max="100" range-step="1" range-value="50">
 *     <variable>MY_VSCALE</variable>
 *     <action signal="value-changed">echo $MY_VSCALE</action>
 *   </vscale>
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
#include "widget_vscale.h"

static void widget_vscale_input_by_command(variable *var, char *command);
static void widget_vscale_input_by_file(variable *var, char *filename);
static void widget_vscale_input_by_items(variable *var);

void widget_vscale_clear(variable *var)
{
    gtk_range_set_value(GTK_RANGE(var->Widget), 0.0);
}

GtkWidget *widget_vscale_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget *widget;
    gdouble    range_min = 0, range_max = 100, range_step = 1, range_value = 0;
    gchar     *value;

    if (attr) {
        if (!(value = get_tag_attribute(attr, "range-min")))
             value = get_tag_attribute(attr, "scale-min");
        if (value) range_min = atof(value);

        if (!(value = get_tag_attribute(attr, "range-max")))
             value = get_tag_attribute(attr, "scale-max");
        if (value) range_max = atof(value);

        if (!(value = get_tag_attribute(attr, "range-step")))
             value = get_tag_attribute(attr, "scale-step");
        if (value) range_step = atof(value);

        if (!(value = get_tag_attribute(attr, "range-value")))
             value = get_tag_attribute(attr, "scale-value");
        if (value) range_value = atof(value);
    }

    widget = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL,
                                      range_min, range_max, range_step);
    gtk_range_set_value(GTK_RANGE(widget), range_value);
    gtk_scale_set_draw_value(GTK_SCALE(widget), TRUE);
    gtk_widget_set_vexpand(widget, TRUE);

    return widget;
}

gchar *widget_vscale_envvar_all_construct(variable *var)
{
    return NULL;
}

gchar *widget_vscale_envvar_construct(GtkWidget *widget)
{
    gdouble value = gtk_range_get_value(GTK_RANGE(widget));
    return g_strdup_printf("%g", value);
}

void widget_vscale_fileselect(variable *var, const char *name, const char *value)
{
    g_warning("%s(): Fileselect not implemented.", __func__);
}

void widget_vscale_refresh(variable *var)
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
            widget_vscale_input_by_command(var, act + 8);
        if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
            if (!initialised)
                widget_file_monitor_try_create(var, act + 5);
            widget_vscale_input_by_file(var, act + 5);
        }
        act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
    }

    if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
        widget_vscale_input_by_items(var);

    if (!initialised) {
        if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
            act = attributeset_get_first(&element, var->Attributes, ATTR_DEFAULT);
            if (act) gtk_range_set_value(GTK_RANGE(var->Widget), atof(act));
        }
        if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
            (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
            gtk_widget_set_sensitive(var->Widget, FALSE);

        g_signal_connect(G_OBJECT(var->Widget), "value-changed",
            G_CALLBACK(on_any_widget_changed_event), (gpointer)var->Attributes);
    }
}

void widget_vscale_removeselected(variable *var)
{
    g_warning("%s(): Not implemented.", __func__);
}

void widget_vscale_save(variable *var)
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
        fprintf(outfile, "%g\n", gtk_range_get_value(GTK_RANGE(var->Widget)));
        fclose(outfile);
    }
}

static void widget_vscale_input_by_command(variable *var, char *command)
{
    FILE  *infile;
    gchar  line[512];
    if ((infile = widget_opencommand(command))) {
        if (fgets(line, sizeof(line), infile))
            gtk_range_set_value(GTK_RANGE(var->Widget), atof(line));
        fclose(infile);
    }
}

static void widget_vscale_input_by_file(variable *var, char *filename)
{
    FILE  *infile;
    gchar  line[512];
    if ((infile = fopen(filename, "r"))) {
        if (fgets(line, sizeof(line), infile))
            gtk_range_set_value(GTK_RANGE(var->Widget), atof(line));
        fclose(infile);
    }
}

static void widget_vscale_input_by_items(variable *var)
{
    g_warning("%s(): <item> not implemented for vscale.", __func__);
}
