/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_progressbar.h — Widget progressbar FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_PROGRESSBAR_H
#define WIDGET_PROGRESSBAR_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_progressbar_clear(variable *var);
GtkWidget *widget_progressbar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_progressbar_envvar_all_construct(variable *var);
gchar     *widget_progressbar_envvar_construct(GtkWidget *widget);
void       widget_progressbar_fileselect(variable *var, const char *name, const char *value);
void       widget_progressbar_refresh(variable *var);
void       widget_progressbar_removeselected(variable *var);
void       widget_progressbar_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
