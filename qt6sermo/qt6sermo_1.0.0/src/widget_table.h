/*
 * widget_table.h — Tableau Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_TABLE_H
#define WIDGET_TABLE_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_table_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_table_envvar_construct(GtkWidget *widget);
gchar     *widget_table_envvar_all_construct(variable *var);
void       widget_table_clear(variable *var);
void       widget_table_refresh(variable *var);
void       widget_table_fileselect(variable *var, const char *name, const char *value);
void       widget_table_removeselected(variable *var);
void       widget_table_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_TABLE_H */
