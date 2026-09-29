/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_text.h — Widget text SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_TEXT_H
#define WIDGET_TEXT_H

#include "sdl3-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_text_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_text_envvar_construct(GtkWidget *widget);
gchar     *widget_text_envvar_all_construct(variable *var);
void       widget_text_clear(variable *var);
void       widget_text_refresh(variable *var);
void       widget_text_fileselect(variable *var, const char *name, const char *value);
void       widget_text_removeselected(variable *var);
void       widget_text_save(variable *var);

#endif /* WIDGET_TEXT_H */
