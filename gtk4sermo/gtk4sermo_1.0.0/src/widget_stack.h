/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_stack.h — GTK4-native widget (stack)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_STACK_H
#define WIDGET_STACK_H

void       widget_stack_clear(variable *var);
GtkWidget *widget_stack_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_stack_envvar_all_construct(variable *var);
gchar     *widget_stack_envvar_construct(GtkWidget *widget);
void       widget_stack_fileselect(variable *var, const char *name, const char *value);
void       widget_stack_refresh(variable *var);
void       widget_stack_removeselected(variable *var);
void       widget_stack_save(variable *var);

#endif
