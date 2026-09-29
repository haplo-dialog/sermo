/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_edit.h — Widget edit ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_EDIT_H
#define WIDGET_EDIT_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_edit_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_edit_envvar_construct(GtkWidget *widget);
gchar     *widget_edit_envvar_all_construct(variable *var);
void       widget_edit_clear(variable *var);
void       widget_edit_refresh(variable *var);
void       widget_edit_fileselect(variable *var, const char *name, const char *value);
void       widget_edit_removeselected(variable *var);
void       widget_edit_save(variable *var);

#endif /* WIDGET_EDIT_H */
