/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* PASSWORD.h — Widget <password> SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_PASSWORD_H
#define WIDGET_PASSWORD_H
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "variables.h"
#include "attributes.h"
#include "tag_attributes.h"
GtkWidget *widget_password_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_password_envvar_construct(GtkWidget *widget);
gchar     *widget_password_envvar_all_construct(variable *var);
void       widget_password_clear(variable *var);
void       widget_password_refresh(variable *var);
void       widget_password_fileselect(variable *var, const char *name, const char *value);
void       widget_password_removeselected(variable *var);
void       widget_password_save(variable *var);
#endif /* WIDGET_PASSWORD_H */
