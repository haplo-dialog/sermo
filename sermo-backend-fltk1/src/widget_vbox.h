/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_vbox.h — Conteneur vertical FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_VBOX_H
#define WIDGET_VBOX_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_vbox_clear(variable *var);
GtkWidget *widget_vbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_vbox_envvar_all_construct(variable *var);
gchar     *widget_vbox_envvar_construct(GtkWidget *widget);
void       widget_vbox_fileselect(variable *var, const char *name, const char *value);
void       widget_vbox_refresh(variable *var);
void       widget_vbox_removeselected(variable *var);
void       widget_vbox_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
