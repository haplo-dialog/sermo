/*
 * widget_menubar.h — Barre de menus Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_MENUBAR_H
#define WIDGET_MENUBAR_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_menubar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
/* <menu> : non porté nativement sur Qt6 (stub dans widget_stubs.cpp). Le core
 * (automaton.c) appelle widget_menu_create ; l'export passe par menuitem. */
GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_menubar_envvar_construct(GtkWidget *widget);
gchar     *widget_menubar_envvar_all_construct(variable *var);
void       widget_menubar_clear(variable *var);
void       widget_menubar_refresh(variable *var);
void       widget_menubar_fileselect(variable *var, const char *name, const char *value);
void       widget_menubar_removeselected(variable *var);
void       widget_menubar_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_MENUBAR_H */
