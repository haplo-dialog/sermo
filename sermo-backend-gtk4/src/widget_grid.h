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
 * <grid columns="N"> range ses enfants en flot : dans l'ordre du document, de
 * gauche à droite, retour à la ligne tous les N enfants. Les colonnes sont
 * ALIGNÉES entre elles — c'est ce que l'empilement de <hbox> dans une <vbox>
 * ne sait pas faire.
 *
 * ⚠️ À ne pas confondre avec <table>, qui est la LISTE à colonnes héritée de
 * gtkdialog (des données), pas une mise en page.
 */
#ifndef WIDGET_GRID_H
#define WIDGET_GRID_H

#ifndef QT6_COMPAT_H
#include <gtk/gtk.h>   /* variante GTK4 */
#endif
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_grid_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_grid_envvar_construct(GtkWidget *widget);
gchar     *widget_grid_envvar_all_construct(variable *var);
void       widget_grid_clear(variable *var);
void       widget_grid_refresh(variable *var);
void       widget_grid_fileselect(variable *var, const char *name, const char *value);
void       widget_grid_removeselected(variable *var);
void       widget_grid_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_GRID_H */
