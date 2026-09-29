/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_linkbutton.h — Widget linkbutton FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_LINKBUTTON_H
#define WIDGET_LINKBUTTON_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_linkbutton_clear(variable *var);
GtkWidget *widget_linkbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_linkbutton_envvar_all_construct(variable *var);
gchar     *widget_linkbutton_envvar_construct(GtkWidget *widget);
void       widget_linkbutton_fileselect(variable *var, const char *name, const char *value);
void       widget_linkbutton_refresh(variable *var);
void       widget_linkbutton_removeselected(variable *var);
void       widget_linkbutton_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
