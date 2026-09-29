/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_vseparator.h — Widget vseparator FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_VSEPARATOR_H
#define WIDGET_VSEPARATOR_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_vseparator_clear(variable *var);
GtkWidget *widget_vseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_vseparator_envvar_all_construct(variable *var);
gchar     *widget_vseparator_envvar_construct(GtkWidget *widget);
void       widget_vseparator_fileselect(variable *var, const char *name, const char *value);
void       widget_vseparator_refresh(variable *var);
void       widget_vseparator_removeselected(variable *var);
void       widget_vseparator_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
