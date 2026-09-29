/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_eventbox.h — stub SDL3 (widget non encore porté)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le tag <eventbox> est reconnu par le lexer/parser hérité, mais n'a pas
 * encore d'implémentation native ncurses (terminal). Ces déclarations évitent
 * l'échec de compilation/link ; les corps (widget_stubs.c) sont des no-ops
 * sûrs qui avertissent une fois sur stderr.
 */
#ifndef WIDGET_EVENTBOX_H
#define WIDGET_EVENTBOX_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_eventbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_eventbox_envvar_construct(GtkWidget *widget);
gchar     *widget_eventbox_envvar_all_construct(variable *var);
void       widget_eventbox_clear(variable *var);
void       widget_eventbox_refresh(variable *var);
void       widget_eventbox_fileselect(variable *var, const char *name, const char *value);
void       widget_eventbox_removeselected(variable *var);
void       widget_eventbox_save(variable *var);

#endif /* WIDGET_EVENTBOX_H */
