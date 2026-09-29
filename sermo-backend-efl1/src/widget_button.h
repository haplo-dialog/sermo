/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_button.h — Widget button EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_BUTTON_H
#define WIDGET_BUTTON_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_button_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_button_envvar_construct(GtkWidget *widget);
gchar     *widget_button_envvar_all_construct(variable *var);
void       widget_button_clear(variable *var);
void       widget_button_refresh(variable *var);
void       widget_button_fileselect(variable *var, const char *name, const char *value);
void       widget_button_removeselected(variable *var);
void       widget_button_save(variable *var);

#endif /* WIDGET_BUTTON_H */
