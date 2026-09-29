/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_TREE_H
#define WIDGET_TREE_H
#include "fltk-compat.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "variables.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_tree_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_tree_envvar_construct(GtkWidget *w);
gchar     *widget_tree_envvar_all_construct(variable *v);
void       widget_tree_clear(variable *v);
void       widget_tree_refresh(variable *v);
void       widget_tree_fileselect(variable *v, const char *n, const char *val);
void       widget_tree_removeselected(variable *v);
void       widget_tree_save(variable *v);
#ifdef __cplusplus
}
#endif
#endif /* WIDGET_TREE_H */
