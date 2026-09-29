/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_filechooser.h — Widget filechooser FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_FILECHOOSER_H
#define WIDGET_FILECHOOSER_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_filechooser_clear(variable *var);
GtkWidget *widget_filechooser_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_filechooser_envvar_all_construct(variable *var);
gchar     *widget_filechooser_envvar_construct(GtkWidget *widget);
void       widget_filechooser_fileselect(variable *var, const char *name, const char *value);
void       widget_filechooser_refresh(variable *var);
void       widget_filechooser_removeselected(variable *var);
void       widget_filechooser_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
