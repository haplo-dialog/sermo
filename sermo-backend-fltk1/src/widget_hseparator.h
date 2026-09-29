/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hseparator.h — Widget hseparator FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_HSEPARATOR_H
#define WIDGET_HSEPARATOR_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_hseparator_clear(variable *var);
GtkWidget *widget_hseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_hseparator_envvar_all_construct(variable *var);
gchar     *widget_hseparator_envvar_construct(GtkWidget *widget);
void       widget_hseparator_fileselect(variable *var, const char *name, const char *value);
void       widget_hseparator_refresh(variable *var);
void       widget_hseparator_removeselected(variable *var);
void       widget_hseparator_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
