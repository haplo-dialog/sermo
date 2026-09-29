/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_checkbox.h — Widget checkbox EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_CHECKBOX_H
#define WIDGET_CHECKBOX_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_checkbox_envvar_construct(GtkWidget *widget);
gchar     *widget_checkbox_envvar_all_construct(variable *var);
void       widget_checkbox_clear(variable *var);
void       widget_checkbox_refresh(variable *var);
void       widget_checkbox_fileselect(variable *var, const char *name, const char *value);
void       widget_checkbox_removeselected(variable *var);
void       widget_checkbox_save(variable *var);

#endif /* WIDGET_CHECKBOX_H */
