/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_expander.h — Widget expander FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_EXPANDER_H
#define WIDGET_EXPANDER_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_expander_clear(variable *var);
GtkWidget *widget_expander_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_expander_envvar_all_construct(variable *var);
gchar     *widget_expander_envvar_construct(GtkWidget *widget);
void       widget_expander_fileselect(variable *var, const char *name, const char *value);
void       widget_expander_refresh(variable *var);
void       widget_expander_removeselected(variable *var);
void       widget_expander_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
