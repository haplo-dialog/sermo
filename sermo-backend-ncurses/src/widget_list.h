/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_list.h — Widget list ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_LIST_H
#define WIDGET_LIST_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_list_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_list_envvar_construct(GtkWidget *widget);
gchar     *widget_list_envvar_all_construct(variable *var);
void       widget_list_clear(variable *var);
void       widget_list_refresh(variable *var);
void       widget_list_fileselect(variable *var, const char *name, const char *value);
void       widget_list_removeselected(variable *var);
void       widget_list_save(variable *var);

#endif /* WIDGET_LIST_H */
