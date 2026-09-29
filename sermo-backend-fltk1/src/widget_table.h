/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_table.h — Tableau FLTK (Fl_Table sous-classée)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */

#ifndef WIDGET_TABLE_H
#define WIDGET_TABLE_H

#include "fltk-compat.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "variables.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_table_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_table_envvar_construct(GtkWidget *widget);
gchar     *widget_table_envvar_all_construct(variable *var);
void       widget_table_clear(variable *var);
void       widget_table_refresh(variable *var);
void       widget_table_fileselect(variable *var, const char *n, const char *v);
void       widget_table_removeselected(variable *var);
void       widget_table_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_TABLE_H */
