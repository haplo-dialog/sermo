/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* LEVELBAR.h — Widget <levelbar> ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_LEVELBAR_H
#define WIDGET_LEVELBAR_H
#include "ncurses-compat.h"
#include "dialog_state.h"
#include "variables.h"
#include "attributes.h"
#include "tag_attributes.h"
GtkWidget *widget_levelbar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_levelbar_envvar_construct(GtkWidget *widget);
gchar     *widget_levelbar_envvar_all_construct(variable *var);
void       widget_levelbar_clear(variable *var);
void       widget_levelbar_refresh(variable *var);
void       widget_levelbar_fileselect(variable *var, const char *name, const char *value);
void       widget_levelbar_removeselected(variable *var);
void       widget_levelbar_save(variable *var);
#endif /* WIDGET_LEVELBAR_H */
