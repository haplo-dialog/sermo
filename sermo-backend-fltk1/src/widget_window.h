/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_window.h — Fenêtre principale FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */

#ifndef WIDGET_WINDOW_H
#define WIDGET_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

void      widget_window_clear(variable *var);
GtkWidget *widget_window_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_window_envvar_all_construct(variable *var);
gchar     *widget_window_envvar_construct(GtkWidget *widget);
void      widget_window_fileselect(variable *var, const char *name, const char *value);
void      widget_window_refresh(variable *var);
void      widget_window_removeselected(variable *var);
void      widget_window_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_WINDOW_H */
