/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubutton.h — Bouton qui déroule un menu
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <menubutton> : un menu LOCAL, là où <menubar> n'existe qu'en tête de
 * fenêtre. Ses enfants sont des <menuitem> ordinaires — le tag ne réinvente
 * rien, il réutilise ce qui existe.
 *
 * Export : le libellé du DERNIER élément choisi ; chaîne vide avant tout
 * choix. (Aucun précédent dans le langage hérité : la sémantique est
 * documentée dans le manuel.)
 */
#ifndef WIDGET_MENUBUTTON_H
#define WIDGET_MENUBUTTON_H

#include "sdl3-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_menubutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_menubutton_envvar_construct(GtkWidget *widget);
gchar     *widget_menubutton_envvar_all_construct(variable *var);
void       widget_menubutton_clear(variable *var);
void       widget_menubutton_refresh(variable *var);
void       widget_menubutton_fileselect(variable *var, const char *name, const char *value);
void       widget_menubutton_removeselected(variable *var);
void       widget_menubutton_save(variable *var);

#endif /* WIDGET_MENUBUTTON_H */
