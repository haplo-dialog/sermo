/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_pixmap.h — Widget pixmap FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_PIXMAP_H
#define WIDGET_PIXMAP_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_pixmap_clear(variable *var);
GtkWidget *widget_pixmap_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_pixmap_envvar_all_construct(variable *var);
gchar     *widget_pixmap_envvar_construct(GtkWidget *widget);
void       widget_pixmap_fileselect(variable *var, const char *name, const char *value);
void       widget_pixmap_refresh(variable *var);
void       widget_pixmap_removeselected(variable *var);
void       widget_pixmap_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
