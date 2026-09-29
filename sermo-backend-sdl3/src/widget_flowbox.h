/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_flowbox.h — Rangement automatique en lignes
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <flowbox> : des enfants qui se RANGENT TOUT SEULS — autant par ligne que la
 * largeur en permet, et le reste passe à la ligne. Là où <grid> fixe le nombre
 * de colonnes, celui-ci s'adapte à la place disponible.
 *   min-children-per-line= / max-children-per-line=
 *   column-spacing= / row-spacing=
 *   selection-mode= none (défaut) | single | browse | multiple
 *
 * Export : l'index de l'enfant sélectionné, chaîne vide s'il n'y en a pas.
 */
#ifndef WIDGET_FLOWBOX_H
#define WIDGET_FLOWBOX_H

#include "sdl3-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_flowbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_flowbox_envvar_construct(GtkWidget *widget);
gchar     *widget_flowbox_envvar_all_construct(variable *var);
void       widget_flowbox_clear(variable *var);
void       widget_flowbox_refresh(variable *var);
void       widget_flowbox_fileselect(variable *var, const char *name, const char *value);
void       widget_flowbox_removeselected(variable *var);
void       widget_flowbox_save(variable *var);

#endif /* WIDGET_FLOWBOX_H */
