/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_terminal.h — Widget terminal EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_TERMINAL_H
#define WIDGET_TERMINAL_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_terminal_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_terminal_envvar_construct(GtkWidget *widget);
gchar     *widget_terminal_envvar_all_construct(variable *var);
void       widget_terminal_clear(variable *var);
void       widget_terminal_refresh(variable *var);
void       widget_terminal_fileselect(variable *var, const char *name, const char *value);
void       widget_terminal_removeselected(variable *var);
void       widget_terminal_save(variable *var);

#endif /* WIDGET_TERMINAL_H */
