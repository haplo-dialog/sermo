/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_edit.h — Widget edit FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_EDIT_H
#define WIDGET_EDIT_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_edit_clear(variable *var);
GtkWidget *widget_edit_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_edit_envvar_all_construct(variable *var);
gchar     *widget_edit_envvar_construct(GtkWidget *widget);
void       widget_edit_fileselect(variable *var, const char *name, const char *value);
void       widget_edit_refresh(variable *var);
void       widget_edit_removeselected(variable *var);
void       widget_edit_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
