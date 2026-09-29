/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hseparator.h — Widget hseparator EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_HSEPARATOR_H
#define WIDGET_HSEPARATOR_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_hseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_hseparator_envvar_construct(GtkWidget *widget);
gchar     *widget_hseparator_envvar_all_construct(variable *var);
void       widget_hseparator_clear(variable *var);
void       widget_hseparator_refresh(variable *var);
void       widget_hseparator_fileselect(variable *var, const char *name, const char *value);
void       widget_hseparator_removeselected(variable *var);
void       widget_hseparator_save(variable *var);

#endif /* WIDGET_HSEPARATOR_H */
