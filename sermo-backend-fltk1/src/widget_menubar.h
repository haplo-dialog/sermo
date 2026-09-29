/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_menubar.h — Barre de menus FLTK (Fl_Menu_Bar)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */

#ifndef WIDGET_MENUBAR_H
#define WIDGET_MENUBAR_H

#include "fltk-compat.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "variables.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_menubar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_menubar_envvar_construct(GtkWidget *widget);
gchar     *widget_menubar_envvar_all_construct(variable *var);
void       widget_menubar_clear(variable *var);
void       widget_menubar_refresh(variable *var);
void       widget_menubar_fileselect(variable *var, const char *n, const char *v);
void       widget_menubar_removeselected(variable *var);
void       widget_menubar_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_MENUBAR_H */
