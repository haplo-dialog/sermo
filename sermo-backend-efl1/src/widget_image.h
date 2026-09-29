/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_image.h — stub EFL (widget non encore porté)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le tag <image> est reconnu par le lexer/parser hérité, mais n'a pas
 * encore d'implémentation native EFL/Elementary. Ces déclarations évitent
 * l'échec de compilation/link ; les corps (widget_stubs.c) sont des no-ops
 * sûrs qui avertissent une fois sur stderr.
 */
#ifndef WIDGET_IMAGE_H
#define WIDGET_IMAGE_H

#include "efl-compat.h"
#include "widgets.h"

GtkWidget *widget_image_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_image_envvar_construct(GtkWidget *widget);
gchar     *widget_image_envvar_all_construct(variable *var);
void       widget_image_clear(variable *var);
void       widget_image_refresh(variable *var);
void       widget_image_fileselect(variable *var, const char *name, const char *value);
void       widget_image_removeselected(variable *var);
void       widget_image_save(variable *var);

#endif /* WIDGET_IMAGE_H */
