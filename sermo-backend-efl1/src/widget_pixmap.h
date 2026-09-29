/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_pixmap.h — Widget pixmap EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_PIXMAP_H
#define WIDGET_PIXMAP_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_pixmap_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_pixmap_envvar_construct(GtkWidget *widget);
gchar     *widget_pixmap_envvar_all_construct(variable *var);
void       widget_pixmap_clear(variable *var);
void       widget_pixmap_refresh(variable *var);
void       widget_pixmap_fileselect(variable *var, const char *name, const char *value);
void       widget_pixmap_removeselected(variable *var);
void       widget_pixmap_save(variable *var);

#endif /* WIDGET_PIXMAP_H */
