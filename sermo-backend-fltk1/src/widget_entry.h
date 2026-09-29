/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_entry.h — Widget entry FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_ENTRY_H
#define WIDGET_ENTRY_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_entry_clear(variable *var);
GtkWidget *widget_entry_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_entry_envvar_all_construct(variable *var);
gchar     *widget_entry_envvar_construct(GtkWidget *widget);
void       widget_entry_fileselect(variable *var, const char *name, const char *value);
void       widget_entry_refresh(variable *var);
void       widget_entry_removeselected(variable *var);
void       widget_entry_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
