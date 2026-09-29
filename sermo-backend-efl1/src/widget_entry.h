/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_entry.h — Widget entry EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_ENTRY_H
#define WIDGET_ENTRY_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_entry_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_entry_envvar_construct(GtkWidget *widget);
gchar     *widget_entry_envvar_all_construct(variable *var);
void       widget_entry_clear(variable *var);
void       widget_entry_refresh(variable *var);
void       widget_entry_fileselect(variable *var, const char *name, const char *value);
void       widget_entry_removeselected(variable *var);
void       widget_entry_save(variable *var);

#endif /* WIDGET_ENTRY_H */
