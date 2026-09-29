/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* DRAWINGAREA.h — Widget <drawingarea> SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_DRAWINGAREA_H
#define WIDGET_DRAWINGAREA_H
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "variables.h"
#include "attributes.h"
#include "tag_attributes.h"
GtkWidget *widget_drawingarea_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_drawingarea_envvar_construct(GtkWidget *widget);
gchar     *widget_drawingarea_envvar_all_construct(variable *var);
void       widget_drawingarea_clear(variable *var);
void       widget_drawingarea_refresh(variable *var);
void       widget_drawingarea_fileselect(variable *var, const char *name, const char *value);
void       widget_drawingarea_removeselected(variable *var);
void       widget_drawingarea_save(variable *var);
#endif /* WIDGET_DRAWINGAREA_H */
