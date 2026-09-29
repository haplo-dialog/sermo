/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_overlay.h — Enfants empilés l'un sur l'autre
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <overlay> : des enfants EMPILÉS l'un sur l'autre. Le PREMIER est le fond ;
 * les suivants flottent par-dessus (badge, indicateur, bouton posé sur une
 * image). Aucun autre conteneur du langage ne superpose.
 *
 * Export : chaîne vide — c'est un conteneur.
 */
#ifndef WIDGET_OVERLAY_H
#define WIDGET_OVERLAY_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_overlay_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_overlay_envvar_construct(GtkWidget *widget);
gchar     *widget_overlay_envvar_all_construct(variable *var);
void       widget_overlay_clear(variable *var);
void       widget_overlay_refresh(variable *var);
void       widget_overlay_fileselect(variable *var, const char *name, const char *value);
void       widget_overlay_removeselected(variable *var);
void       widget_overlay_save(variable *var);

#endif /* WIDGET_OVERLAY_H */
