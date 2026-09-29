/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* SWITCH.h — Widget <switch> SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_SWITCH_H
#define WIDGET_SWITCH_H
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "variables.h"
#include "attributes.h"
#include "tag_attributes.h"
GtkWidget *widget_switch_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_switch_envvar_construct(GtkWidget *widget);
gchar     *widget_switch_envvar_all_construct(variable *var);
void       widget_switch_clear(variable *var);
void       widget_switch_refresh(variable *var);
void       widget_switch_fileselect(variable *var, const char *name, const char *value);
void       widget_switch_removeselected(variable *var);
void       widget_switch_save(variable *var);
#endif /* WIDGET_SWITCH_H */
