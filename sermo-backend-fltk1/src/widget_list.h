/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_list.h — Widget list FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_LIST_H
#define WIDGET_LIST_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_list_clear(variable *var);
GtkWidget *widget_list_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_list_envvar_all_construct(variable *var);
gchar     *widget_list_envvar_construct(GtkWidget *widget);
void       widget_list_fileselect(variable *var, const char *name, const char *value);
void       widget_list_refresh(variable *var);
void       widget_list_removeselected(variable *var);
void       widget_list_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
