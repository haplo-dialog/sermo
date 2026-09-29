/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_tree.h — Widget tree EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_TREE_H
#define WIDGET_TREE_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_tree_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_tree_envvar_construct(GtkWidget *widget);
gchar     *widget_tree_envvar_all_construct(variable *var);
void       widget_tree_clear(variable *var);
void       widget_tree_refresh(variable *var);
void       widget_tree_fileselect(variable *var, const char *name, const char *value);
void       widget_tree_removeselected(variable *var);
void       widget_tree_save(variable *var);

#endif /* WIDGET_TREE_H */
