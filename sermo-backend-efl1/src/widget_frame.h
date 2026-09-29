/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_frame.h — Widget frame EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_FRAME_H
#define WIDGET_FRAME_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_frame_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_frame_envvar_construct(GtkWidget *widget);
gchar     *widget_frame_envvar_all_construct(variable *var);
void       widget_frame_clear(variable *var);
void       widget_frame_refresh(variable *var);
void       widget_frame_fileselect(variable *var, const char *name, const char *value);
void       widget_frame_removeselected(variable *var);
void       widget_frame_save(variable *var);

#endif /* WIDGET_FRAME_H */
