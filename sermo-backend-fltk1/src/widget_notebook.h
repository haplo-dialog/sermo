/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_notebook.h — Widget notebook FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_NOTEBOOK_H
#define WIDGET_NOTEBOOK_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_notebook_clear(variable *var);
GtkWidget *widget_notebook_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_notebook_envvar_all_construct(variable *var);
gchar     *widget_notebook_envvar_construct(GtkWidget *widget);
void       widget_notebook_fileselect(variable *var, const char *name, const char *value);
void       widget_notebook_refresh(variable *var);
void       widget_notebook_removeselected(variable *var);
void       widget_notebook_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
