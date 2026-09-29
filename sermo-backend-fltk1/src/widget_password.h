/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_PASSWORD_H
#define WIDGET_PASSWORD_H
#include "fltk-compat.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "variables.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_password_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_password_envvar_construct(GtkWidget *w);
gchar     *widget_password_envvar_all_construct(variable *v);
void       widget_password_clear(variable *v);
void       widget_password_refresh(variable *v);
void       widget_password_fileselect(variable *v, const char *n, const char *val);
void       widget_password_removeselected(variable *v);
void       widget_password_save(variable *v);
#ifdef __cplusplus
}
#endif
#endif /* WIDGET_PASSWORD_H */
