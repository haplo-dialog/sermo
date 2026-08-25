/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_revealer.h — GTK4-native widget (revealer)
 * gtk4sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_REVEALER_H
#define WIDGET_REVEALER_H

void       widget_revealer_clear(variable *var);
GtkWidget *widget_revealer_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_revealer_envvar_all_construct(variable *var);
gchar     *widget_revealer_envvar_construct(GtkWidget *widget);
void       widget_revealer_fileselect(variable *var, const char *name, const char *value);
void       widget_revealer_refresh(variable *var);
void       widget_revealer_removeselected(variable *var);
void       widget_revealer_save(variable *var);

#endif
