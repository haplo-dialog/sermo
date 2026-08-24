/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_togglebutton.h — GtkToggleButton (GTK4)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_TOGGLEBUTTON_H
#define WIDGET_TOGGLEBUTTON_H

void       widget_togglebutton_clear(variable *var);
GtkWidget *widget_togglebutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_togglebutton_envvar_all_construct(variable *var);
gchar     *widget_togglebutton_envvar_construct(GtkWidget *widget);
void       widget_togglebutton_fileselect(variable *var, const char *name, const char *value);
void       widget_togglebutton_refresh(variable *var);
void       widget_togglebutton_removeselected(variable *var);
void       widget_togglebutton_save(variable *var);

#endif
