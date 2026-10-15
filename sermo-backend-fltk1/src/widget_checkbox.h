/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_checkbox.h — Widget checkbox FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef WIDGET_CHECKBOX_H
#define WIDGET_CHECKBOX_H
#ifdef __cplusplus
extern "C" {
#endif
void       widget_checkbox_clear(variable *var);
GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_checkbox_envvar_all_construct(variable *var);
gchar     *widget_checkbox_envvar_construct(GtkWidget *widget);
void       widget_checkbox_fileselect(variable *var, const char *name, const char *value);
void       widget_checkbox_refresh(variable *var);
void       widget_checkbox_removeselected(variable *var);
void       widget_checkbox_save(variable *var);
/* Widgets à bascule (case, bouton à bascule, radio) : joue leurs actions à
 * chaque changement d'état. chef = 1er radio du groupe, NULL sinon. */
void       fltk1_bascule_brancher(GtkWidget *bouton, AttributeSet *Attr, GtkWidget *chef);
void       fltk1_interrupteur_brancher(GtkWidget *bouton, AttributeSet *Attr);   /* <switch> */
void       fltk1_bascule_lire(variable *var);   /* <input> d'une bascule, à chaque refresh */
void       fltk1_radio_coche(GtkWidget *rb);   /* widget_radiobutton.cpp : le coché du groupe ouvert */
#ifdef __cplusplus
}
#endif
#endif
