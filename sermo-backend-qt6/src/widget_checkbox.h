/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_CHECKBOX_H
#define WIDGET_CHECKBOX_H
#include "gtk3d.h"
#include "attributes.h"
#include "tag_attributes.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar *widget_checkbox_envvar_construct(GtkWidget *widget);
gchar *widget_checkbox_envvar_all_construct(variable *var);
void   widget_checkbox_clear(variable *var);
void   widget_checkbox_refresh(variable *var);
void   widget_checkbox_fileselect(variable *var, const char *n, const char *v);
void   widget_checkbox_removeselected(variable *var);
void   widget_checkbox_save(variable *var);
/* Widgets à bascule (case, bouton à bascule, radio) : joue leurs actions à
 * chaque changement d'état ; connus du crochet « activate: ». */
void   qt6_bascule_brancher(GtkWidget *bouton, AttributeSet *Attr);
void   qt6_interrupteur_brancher(GtkWidget *bouton, AttributeSet *Attr);   /* <switch> */
void   qt6_bascule_lire(variable *var);   /* <input> d'une bascule, à chaque refresh */
#ifdef __cplusplus
}
#endif
#endif
