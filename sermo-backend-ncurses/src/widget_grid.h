/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_grid.h — Conteneur de mise en page en tableau
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <grid columns="N"> range ses enfants EN FLOT : ordre du document, retour à
 * la ligne tous les N. Les colonnes sont ALIGNÉES d'une rangée à l'autre —
 * ce que l'empilement de <hbox> dans une <vbox> ne sait pas faire.
 *
 * ⚠️ À ne pas confondre avec <table>, la liste à colonnes héritée de gtkdialog.
 */
#ifndef WIDGET_GRID_H
#define WIDGET_GRID_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_grid_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_grid_envvar_construct(GtkWidget *widget);
gchar     *widget_grid_envvar_all_construct(variable *var);
void       widget_grid_clear(variable *var);
void       widget_grid_refresh(variable *var);
void       widget_grid_fileselect(variable *var, const char *name, const char *value);
void       widget_grid_removeselected(variable *var);
void       widget_grid_save(variable *var);

#endif /* WIDGET_GRID_H */
