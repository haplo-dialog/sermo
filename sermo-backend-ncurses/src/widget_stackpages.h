/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_stackpages.h — N pages, une seule visible
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <stack page="1" switcher="true"> : chaque enfant est une PAGE ; une seule
 * s'affiche. C'est <notebook> sans les onglets — utile quand c'est le script
 * (ou un assistant) qui décide de la page, pas l'utilisateur.
 *   page=     index de la page initiale, base 0
 *   switcher= true ajoute une rangée de boutons pour changer de page
 *
 * Export : l'index de la page visible, comme <notebook>.
 */
#ifndef WIDGET_STACKPAGES_H
#define WIDGET_STACKPAGES_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_stackpages_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_stackpages_envvar_construct(GtkWidget *widget);
gchar     *widget_stackpages_envvar_all_construct(variable *var);
void       widget_stackpages_clear(variable *var);
void       widget_stackpages_refresh(variable *var);
void       widget_stackpages_fileselect(variable *var, const char *name, const char *value);
void       widget_stackpages_removeselected(variable *var);
void       widget_stackpages_save(variable *var);

#endif /* WIDGET_STACKPAGES_H */
