/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_combobox.h — Widget combobox FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_COMBOBOX_H
#define WIDGET_COMBOBOX_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_combobox_clear(variable *var);
GtkWidget *widget_combobox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_combobox_envvar_all_construct(variable *var);
gchar     *widget_combobox_envvar_construct(GtkWidget *widget);
void       widget_combobox_fileselect(variable *var, const char *name, const char *value);
void       widget_combobox_refresh(variable *var);
void       widget_combobox_removeselected(variable *var);
void       widget_combobox_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
