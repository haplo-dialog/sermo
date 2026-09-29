/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menu.h — Widget menu FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_MENU_H
#define WIDGET_MENU_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_menu_clear(variable *var);
GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_menu_envvar_all_construct(variable *var);
gchar     *widget_menu_envvar_construct(GtkWidget *widget);
void       widget_menu_fileselect(variable *var, const char *name, const char *value);
void       widget_menu_refresh(variable *var);
void       widget_menu_removeselected(variable *var);
void       widget_menu_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
