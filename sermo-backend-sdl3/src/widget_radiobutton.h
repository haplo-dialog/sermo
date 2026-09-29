/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_radiobutton.h — Widget radiobutton SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_RADIOBUTTON_H
#define WIDGET_RADIOBUTTON_H

#include "sdl3-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_radiobutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_radiobutton_envvar_construct(GtkWidget *widget);
gchar     *widget_radiobutton_envvar_all_construct(variable *var);
void       widget_radiobutton_clear(variable *var);
void       widget_radiobutton_refresh(variable *var);
void       widget_radiobutton_fileselect(variable *var, const char *name, const char *value);
void       widget_radiobutton_removeselected(variable *var);
void       widget_radiobutton_save(variable *var);

#endif /* WIDGET_RADIOBUTTON_H */
