/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_pulse.h — stub SDL3 (widget non encore porté)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le tag <pulse> est reconnu par le lexer/parser hérité, mais n'a pas
 * encore d'implémentation native SDL3/ImGui. Ces déclarations évitent
 * l'échec de compilation/link ; les corps (widget_stubs.c) sont des no-ops
 * sûrs qui avertissent une fois sur stderr.
 */
#ifndef WIDGET_PULSE_H
#define WIDGET_PULSE_H

#include "sdl3-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_pulse_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_pulse_envvar_construct(GtkWidget *widget);
gchar     *widget_pulse_envvar_all_construct(variable *var);
void       widget_pulse_clear(variable *var);
void       widget_pulse_refresh(variable *var);
void       widget_pulse_fileselect(variable *var, const char *name, const char *value);
void       widget_pulse_removeselected(variable *var);
void       widget_pulse_save(variable *var);

#endif /* WIDGET_PULSE_H */
