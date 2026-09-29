/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_spinbutton.h — Widget spinbutton FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_SPINBUTTON_H
#define WIDGET_SPINBUTTON_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_spinbutton_clear(variable *var);
GtkWidget *widget_spinbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_spinbutton_envvar_all_construct(variable *var);
gchar     *widget_spinbutton_envvar_construct(GtkWidget *widget);
void       widget_spinbutton_fileselect(variable *var, const char *name, const char *value);
void       widget_spinbutton_refresh(variable *var);
void       widget_spinbutton_removeselected(variable *var);
void       widget_spinbutton_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
