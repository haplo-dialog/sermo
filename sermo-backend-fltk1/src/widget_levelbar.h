/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_levelbar.h — Widget levelbar FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_LEVELBAR_H
#define WIDGET_LEVELBAR_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_levelbar_clear(variable *var);
GtkWidget *widget_levelbar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_levelbar_envvar_all_construct(variable *var);
gchar     *widget_levelbar_envvar_construct(GtkWidget *widget);
void       widget_levelbar_fileselect(variable *var, const char *name, const char *value);
void       widget_levelbar_refresh(variable *var);
void       widget_levelbar_removeselected(variable *var);
void       widget_levelbar_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
