/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_paned.h — Deux zones séparées par une poignée déplaçable
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <paned orientation="horizontal" position="30%"> prend EXACTEMENT deux
 * enfants et laisse l'utilisateur redistribuer la place entre eux.
 *   orientation : horizontal (défaut, deux zones côte à côte, poignée
 *                 verticale) | vertical (l'une au-dessus de l'autre)
 *   position    : position initiale de la poignée, en pixels ou en %
 *   resizable   : true (défaut) | false — poignée figée
 *
 * ⚠️ Un TROISIÈME enfant est REFUSÉ avec un message sur stderr, jamais avalé
 * en silence : l'emballer dans une <vbox> est le geste attendu.
 */
#ifndef WIDGET_PANED_H
#define WIDGET_PANED_H

#include "sdl3-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_paned_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_paned_envvar_construct(GtkWidget *widget);
gchar     *widget_paned_envvar_all_construct(variable *var);
void       widget_paned_clear(variable *var);
void       widget_paned_refresh(variable *var);
void       widget_paned_fileselect(variable *var, const char *name, const char *value);
void       widget_paned_removeselected(variable *var);
void       widget_paned_save(variable *var);

#endif /* WIDGET_PANED_H */
