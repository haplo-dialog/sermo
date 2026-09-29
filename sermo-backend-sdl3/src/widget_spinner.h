/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* SPINNER.h — Widget <spinner> SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_SPINNER_H
#define WIDGET_SPINNER_H
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "variables.h"
#include "attributes.h"
#include "tag_attributes.h"
GtkWidget *widget_spinner_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_spinner_envvar_construct(GtkWidget *widget);
gchar     *widget_spinner_envvar_all_construct(variable *var);
void       widget_spinner_clear(variable *var);
void       widget_spinner_refresh(variable *var);
void       widget_spinner_fileselect(variable *var, const char *name, const char *value);
void       widget_spinner_removeselected(variable *var);
void       widget_spinner_save(variable *var);
#endif /* WIDGET_SPINNER_H */
