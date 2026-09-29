/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_statusbar.h — Widget statusbar FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_STATUSBAR_H
#define WIDGET_STATUSBAR_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_statusbar_clear(variable *var);
GtkWidget *widget_statusbar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_statusbar_envvar_all_construct(variable *var);
gchar     *widget_statusbar_envvar_construct(GtkWidget *widget);
void       widget_statusbar_fileselect(variable *var, const char *name, const char *value);
void       widget_statusbar_refresh(variable *var);
void       widget_statusbar_removeselected(variable *var);
void       widget_statusbar_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
