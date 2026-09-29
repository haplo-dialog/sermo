/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_aspectframe.h — Widget aspectframe FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_ASPECTFRAME_H
#define WIDGET_ASPECTFRAME_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_aspectframe_clear(variable *var);
GtkWidget *widget_aspectframe_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_aspectframe_envvar_all_construct(variable *var);
gchar     *widget_aspectframe_envvar_construct(GtkWidget *widget);
void       widget_aspectframe_fileselect(variable *var, const char *name, const char *value);
void       widget_aspectframe_refresh(variable *var);
void       widget_aspectframe_removeselected(variable *var);
void       widget_aspectframe_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
