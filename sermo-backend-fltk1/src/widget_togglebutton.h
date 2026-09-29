/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_togglebutton.h — Widget togglebutton FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_TOGGLEBUTTON_H
#define WIDGET_TOGGLEBUTTON_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_togglebutton_clear(variable *var);
GtkWidget *widget_togglebutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_togglebutton_envvar_all_construct(variable *var);
gchar     *widget_togglebutton_envvar_construct(GtkWidget *widget);
void       widget_togglebutton_fileselect(variable *var, const char *name, const char *value);
void       widget_togglebutton_refresh(variable *var);
void       widget_togglebutton_removeselected(variable *var);
void       widget_togglebutton_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
