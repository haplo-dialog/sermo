/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_progressbar.h — Widget progressbar ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_PROGRESSBAR_H
#define WIDGET_PROGRESSBAR_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_progressbar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_progressbar_envvar_construct(GtkWidget *widget);
gchar     *widget_progressbar_envvar_all_construct(variable *var);
void       widget_progressbar_clear(variable *var);
void       widget_progressbar_refresh(variable *var);
void       widget_progressbar_fileselect(variable *var, const char *name, const char *value);
void       widget_progressbar_removeselected(variable *var);
void       widget_progressbar_save(variable *var);

#endif /* WIDGET_PROGRESSBAR_H */
