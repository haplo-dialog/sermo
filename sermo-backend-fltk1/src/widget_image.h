/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_image.h — Widget image FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_IMAGE_H
#define WIDGET_IMAGE_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_image_clear(variable *var);
GtkWidget *widget_image_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_image_envvar_all_construct(variable *var);
gchar     *widget_image_envvar_construct(GtkWidget *widget);
void       widget_image_fileselect(variable *var, const char *name, const char *value);
void       widget_image_refresh(variable *var);
void       widget_image_removeselected(variable *var);
void       widget_image_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
