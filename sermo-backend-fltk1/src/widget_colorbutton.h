/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_colorbutton.h — Widget colorbutton FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_COLORBUTTON_H
#define WIDGET_COLORBUTTON_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_colorbutton_clear(variable *var);
GtkWidget *widget_colorbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_colorbutton_envvar_all_construct(variable *var);
gchar     *widget_colorbutton_envvar_construct(GtkWidget *widget);
void       widget_colorbutton_fileselect(variable *var, const char *name, const char *value);
void       widget_colorbutton_refresh(variable *var);
void       widget_colorbutton_removeselected(variable *var);
void       widget_colorbutton_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
