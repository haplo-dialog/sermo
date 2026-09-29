/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hscale.h — Widget hscale FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_HSCALE_H
#define WIDGET_HSCALE_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_hscale_clear(variable *var);
GtkWidget *widget_hscale_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_hscale_envvar_all_construct(variable *var);
gchar     *widget_hscale_envvar_construct(GtkWidget *widget);
void       widget_hscale_fileselect(variable *var, const char *name, const char *value);
void       widget_hscale_refresh(variable *var);
void       widget_hscale_removeselected(variable *var);
void       widget_hscale_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
