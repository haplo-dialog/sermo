/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_flowbox.h — GTK4-native widget (flowbox)
 * gtk4sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_FLOWBOX_H
#define WIDGET_FLOWBOX_H

void       widget_flowbox_clear(variable *var);
GtkWidget *widget_flowbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_flowbox_envvar_all_construct(variable *var);
gchar     *widget_flowbox_envvar_construct(GtkWidget *widget);
void       widget_flowbox_fileselect(variable *var, const char *name, const char *value);
void       widget_flowbox_refresh(variable *var);
void       widget_flowbox_removeselected(variable *var);
void       widget_flowbox_save(variable *var);

#endif
