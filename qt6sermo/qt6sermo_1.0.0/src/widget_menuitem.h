/*
 * widget_menuitem.h — Élément de menu Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_MENUITEM_H
#define WIDGET_MENUITEM_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_menuitem_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_menuitem_envvar_construct(GtkWidget *widget);
gchar     *widget_menuitem_envvar_all_construct(variable *var);
void       widget_menuitem_clear(variable *var);
void       widget_menuitem_refresh(variable *var);
void       widget_menuitem_fileselect(variable *var, const char *name, const char *value);
void       widget_menuitem_removeselected(variable *var);
void       widget_menuitem_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_MENUITEM_H */
