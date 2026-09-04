/*
 * widget_expander.h — Zone dépliable Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_EXPANDER_H
#define WIDGET_EXPANDER_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_expander_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_expander_envvar_construct(GtkWidget *widget);
gchar     *widget_expander_envvar_all_construct(variable *var);
void       widget_expander_clear(variable *var);
void       widget_expander_refresh(variable *var);
void       widget_expander_fileselect(variable *var, const char *name, const char *value);
void       widget_expander_removeselected(variable *var);
void       widget_expander_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_EXPANDER_H */
