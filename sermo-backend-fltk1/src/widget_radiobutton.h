/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_radiobutton.h — Widget radiobutton FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_RADIOBUTTON_H
#define WIDGET_RADIOBUTTON_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_radiobutton_clear(variable *var);
GtkWidget *widget_radiobutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_radiobutton_envvar_all_construct(variable *var);
gchar     *widget_radiobutton_envvar_construct(GtkWidget *widget);
void       widget_radiobutton_fileselect(variable *var, const char *name, const char *value);
void       widget_radiobutton_refresh(variable *var);
void       widget_radiobutton_removeselected(variable *var);
void       widget_radiobutton_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
