/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_table.h — Widget table SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_TABLE_H
#define WIDGET_TABLE_H

#include "sdl3-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_table_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_table_envvar_construct(GtkWidget *widget);
gchar     *widget_table_envvar_all_construct(variable *var);
void       widget_table_clear(variable *var);
void       widget_table_refresh(variable *var);
void       widget_table_fileselect(variable *var, const char *name, const char *value);
void       widget_table_removeselected(variable *var);
void       widget_table_save(variable *var);

#endif /* WIDGET_TABLE_H */
