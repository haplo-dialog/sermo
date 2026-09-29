/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_fontbutton.h — Widget fontbutton SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_FONTBUTTON_H
#define WIDGET_FONTBUTTON_H

#include "sdl3-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_fontbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_fontbutton_envvar_construct(GtkWidget *widget);
gchar     *widget_fontbutton_envvar_all_construct(variable *var);
void       widget_fontbutton_clear(variable *var);
void       widget_fontbutton_refresh(variable *var);
void       widget_fontbutton_fileselect(variable *var, const char *name, const char *value);
void       widget_fontbutton_removeselected(variable *var);
void       widget_fontbutton_save(variable *var);

#endif /* WIDGET_FONTBUTTON_H */
