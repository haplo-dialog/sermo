/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hbox.h — Widget hbox ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_HBOX_H
#define WIDGET_HBOX_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_hbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_hbox_envvar_construct(GtkWidget *widget);
gchar     *widget_hbox_envvar_all_construct(variable *var);
void       widget_hbox_clear(variable *var);
void       widget_hbox_refresh(variable *var);
void       widget_hbox_fileselect(variable *var, const char *name, const char *value);
void       widget_hbox_removeselected(variable *var);
void       widget_hbox_save(variable *var);

#endif /* WIDGET_HBOX_H */
