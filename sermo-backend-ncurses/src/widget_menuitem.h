/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menuitem.h — Widget menuitem ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_MENUITEM_H
#define WIDGET_MENUITEM_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_menuitem_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_menuitem_envvar_construct(GtkWidget *widget);
gchar     *widget_menuitem_envvar_all_construct(variable *var);
void       widget_menuitem_clear(variable *var);
void       widget_menuitem_refresh(variable *var);
void       widget_menuitem_fileselect(variable *var, const char *name, const char *value);
void       widget_menuitem_removeselected(variable *var);
void       widget_menuitem_save(variable *var);

#endif /* WIDGET_MENUITEM_H */
