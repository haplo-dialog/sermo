/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_text.h — Widget text FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_TEXT_H
#define WIDGET_TEXT_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_text_clear(variable *var);
GtkWidget *widget_text_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_text_envvar_all_construct(variable *var);
gchar     *widget_text_envvar_construct(GtkWidget *widget);
void       widget_text_fileselect(variable *var, const char *name, const char *value);
void       widget_text_refresh(variable *var);
void       widget_text_removeselected(variable *var);
void       widget_text_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
