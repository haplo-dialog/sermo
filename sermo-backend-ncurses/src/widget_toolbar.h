/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_toolbar.h — Barre d'actions
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <toolbar> : une rangée de boutons qui se DIT barre d'outils — au thème
 * comme à l'utilisateur. Attributs : orientation (horizontal par défaut |
 * vertical), spacing.
 *
 * ⚠️ Implémentée comme une rangée STYLÉE, pas avec GtkToolbar : GTK 4 l'a
 * retiré et GTK 3 le déprécie. Une boîte + la classe CSS « toolbar » donne le
 * même rendu, le même code des deux côtés, et aucune API condamnée.
 */
#ifndef WIDGET_TOOLBAR_H
#define WIDGET_TOOLBAR_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_toolbar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_toolbar_envvar_construct(GtkWidget *widget);
gchar     *widget_toolbar_envvar_all_construct(variable *var);
void       widget_toolbar_clear(variable *var);
void       widget_toolbar_refresh(variable *var);
void       widget_toolbar_fileselect(variable *var, const char *name, const char *value);
void       widget_toolbar_removeselected(variable *var);
void       widget_toolbar_save(variable *var);

#endif /* WIDGET_TOOLBAR_H */
