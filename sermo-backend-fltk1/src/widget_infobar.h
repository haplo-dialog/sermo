/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_infobar.h — Widget infobar FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_INFOBAR_H
#define WIDGET_INFOBAR_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_infobar_clear(variable *var);
GtkWidget *widget_infobar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_infobar_envvar_all_construct(variable *var);
gchar     *widget_infobar_envvar_construct(GtkWidget *widget);
void       widget_infobar_fileselect(variable *var, const char *name, const char *value);
void       widget_infobar_refresh(variable *var);
void       widget_infobar_removeselected(variable *var);
void       widget_infobar_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
