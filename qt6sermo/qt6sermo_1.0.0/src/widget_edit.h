/*
 * widget_edit.h — Zone de texte multi-lignes éditable Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_EDIT_H
#define WIDGET_EDIT_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_edit_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_edit_envvar_construct(GtkWidget *widget);
gchar     *widget_edit_envvar_all_construct(variable *var);
void       widget_edit_clear(variable *var);
void       widget_edit_refresh(variable *var);
void       widget_edit_fileselect(variable *var, const char *name, const char *value);
void       widget_edit_removeselected(variable *var);
void       widget_edit_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_EDIT_H */
