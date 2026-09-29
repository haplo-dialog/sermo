/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_togglebutton.h — Widget togglebutton EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_TOGGLEBUTTON_H
#define WIDGET_TOGGLEBUTTON_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_togglebutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_togglebutton_envvar_construct(GtkWidget *widget);
gchar     *widget_togglebutton_envvar_all_construct(variable *var);
void       widget_togglebutton_clear(variable *var);
void       widget_togglebutton_refresh(variable *var);
void       widget_togglebutton_fileselect(variable *var, const char *name, const char *value);
void       widget_togglebutton_removeselected(variable *var);
void       widget_togglebutton_save(variable *var);

#endif /* WIDGET_TOGGLEBUTTON_H */
