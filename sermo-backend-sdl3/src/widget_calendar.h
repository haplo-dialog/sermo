/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* CALENDAR.h — Widget <calendar> SDL3/ImGui
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_CALENDAR_H
#define WIDGET_CALENDAR_H
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "variables.h"
#include "attributes.h"
#include "tag_attributes.h"
GtkWidget *widget_calendar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_calendar_envvar_construct(GtkWidget *widget);
gchar     *widget_calendar_envvar_all_construct(variable *var);
void       widget_calendar_clear(variable *var);
void       widget_calendar_refresh(variable *var);
void       widget_calendar_fileselect(variable *var, const char *name, const char *value);
void       widget_calendar_removeselected(variable *var);
void       widget_calendar_save(variable *var);
#endif /* WIDGET_CALENDAR_H */
