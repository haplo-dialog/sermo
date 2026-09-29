/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_vseparator.h — Séparateur vertical Qt6
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_VSEPARATOR_H
#define WIDGET_VSEPARATOR_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_vseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_vseparator_envvar_construct(GtkWidget *widget);
gchar     *widget_vseparator_envvar_all_construct(variable *var);
void       widget_vseparator_clear(variable *var);
void       widget_vseparator_refresh(variable *var);
void       widget_vseparator_fileselect(variable *var, const char *name, const char *value);
void       widget_vseparator_removeselected(variable *var);
void       widget_vseparator_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_VSEPARATOR_H */
