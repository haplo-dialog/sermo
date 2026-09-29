/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_eventbox.h — Widget eventbox FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_EVENTBOX_H
#define WIDGET_EVENTBOX_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_eventbox_clear(variable *var);
GtkWidget *widget_eventbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_eventbox_envvar_all_construct(variable *var);
gchar     *widget_eventbox_envvar_construct(GtkWidget *widget);
void       widget_eventbox_fileselect(variable *var, const char *name, const char *value);
void       widget_eventbox_refresh(variable *var);
void       widget_eventbox_removeselected(variable *var);
void       widget_eventbox_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
