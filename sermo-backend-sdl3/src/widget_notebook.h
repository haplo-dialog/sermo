/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_notebook.h — Widget notebook SDL3/ImGui (immediate mode)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_NOTEBOOK_H
#define WIDGET_NOTEBOOK_H

#include "sdl3-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_notebook_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_notebook_envvar_construct(GtkWidget *widget);
gchar     *widget_notebook_envvar_all_construct(variable *var);
void       widget_notebook_clear(variable *var);
void       widget_notebook_refresh(variable *var);
void       widget_notebook_fileselect(variable *var, const char *name, const char *value);
void       widget_notebook_removeselected(variable *var);
void       widget_notebook_save(variable *var);

#endif /* WIDGET_NOTEBOOK_H */
