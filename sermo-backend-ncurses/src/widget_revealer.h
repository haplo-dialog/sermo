/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_revealer.h — Un enfant qui se montre et se cache
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <revealer> : un enfant qui apparaît et disparaît, avec une transition.
 *   transition= none | crossfade | slide-left | slide-right | slide-up |
 *               slide-down (défaut)
 *   duration=   durée en millisecondes
 *   reveal=     true pour démarrer visible (défaut : caché)
 *
 * Export : « true » ou « false » — l'état courant.
 */
#ifndef WIDGET_REVEALER_H
#define WIDGET_REVEALER_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_revealer_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_revealer_envvar_construct(GtkWidget *widget);
gchar     *widget_revealer_envvar_all_construct(variable *var);
void       widget_revealer_clear(variable *var);
void       widget_revealer_refresh(variable *var);
void       widget_revealer_fileselect(variable *var, const char *name, const char *value);
void       widget_revealer_removeselected(variable *var);
void       widget_revealer_save(variable *var);

#endif /* WIDGET_REVEALER_H */
