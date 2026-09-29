/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubar.h — Widget menubar EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_MENUBAR_H
#define WIDGET_MENUBAR_H

#include "efl-compat.h"
#include "widgets.h"

/* Le tag <menu> n'est pas encore porté ; le core (automaton.c) appelle
 * widget_menu_create. Stub fourni par widget_stubs.c. */
GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type);

GtkWidget *widget_menubar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_menubar_envvar_construct(GtkWidget *widget);
gchar     *widget_menubar_envvar_all_construct(variable *var);
void       widget_menubar_clear(variable *var);
void       widget_menubar_refresh(variable *var);
void       widget_menubar_fileselect(variable *var, const char *name, const char *value);
void       widget_menubar_removeselected(variable *var);
void       widget_menubar_save(variable *var);

#endif /* WIDGET_MENUBAR_H */
