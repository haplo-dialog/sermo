/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_vscale.h — Widget vscale ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_VSCALE_H
#define WIDGET_VSCALE_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_vscale_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_vscale_envvar_construct(GtkWidget *widget);
gchar     *widget_vscale_envvar_all_construct(variable *var);
void       widget_vscale_clear(variable *var);
void       widget_vscale_refresh(variable *var);
void       widget_vscale_fileselect(variable *var, const char *name, const char *value);
void       widget_vscale_removeselected(variable *var);
void       widget_vscale_save(variable *var);

#endif /* WIDGET_VSCALE_H */
