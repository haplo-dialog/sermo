/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_combobox.h — Widget combobox EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_COMBOBOX_H
#define WIDGET_COMBOBOX_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_combobox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_combobox_envvar_construct(GtkWidget *widget);
gchar     *widget_combobox_envvar_all_construct(variable *var);
void       widget_combobox_clear(variable *var);
void       widget_combobox_refresh(variable *var);
void       widget_combobox_fileselect(variable *var, const char *name, const char *value);
void       widget_combobox_removeselected(variable *var);
void       widget_combobox_save(variable *var);

#endif /* WIDGET_COMBOBOX_H */
