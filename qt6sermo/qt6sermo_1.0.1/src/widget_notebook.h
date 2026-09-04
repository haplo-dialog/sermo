/*
 * widget_notebook.h — Onglets Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_NOTEBOOK_H
#define WIDGET_NOTEBOOK_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_notebook_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_notebook_envvar_construct(GtkWidget *widget);
gchar     *widget_notebook_envvar_all_construct(variable *var);
void       widget_notebook_clear(variable *var);
void       widget_notebook_refresh(variable *var);
void       widget_notebook_fileselect(variable *var, const char *name, const char *value);
void       widget_notebook_removeselected(variable *var);
void       widget_notebook_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_NOTEBOOK_H */
