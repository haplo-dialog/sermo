/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_treetable.h — Arbre hiérarchique multi-colonnes FLTK
 * sermo — haplo-dialog — GPL-2.0-or-later */
#pragma once
#ifdef __cplusplus
#include "fltk-compat.h"
#include "automaton.h"
extern "C" {
#endif
GtkWidget *widget_treetable_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_treetable_envvar_construct(GtkWidget *widget);
gchar     *widget_treetable_envvar_all_construct(variable *var);
void       widget_treetable_clear(variable *var);
void       widget_treetable_refresh(variable *var);
void       widget_treetable_fileselect(variable *var, const char *name, const char *value);
void       widget_treetable_removeselected(variable *var);
void       widget_treetable_save(variable *var);
#ifdef __cplusplus
}
#endif
