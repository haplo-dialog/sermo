/*
 * widget_combobox.h — Liste déroulante Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_COMBOBOX_H
#define WIDGET_COMBOBOX_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_combobox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_combobox_envvar_construct(GtkWidget *widget);
gchar     *widget_combobox_envvar_all_construct(variable *var);
void       widget_combobox_clear(variable *var);
void       widget_combobox_refresh(variable *var);
void       widget_combobox_fileselect(variable *var, const char *name, const char *value);
void       widget_combobox_removeselected(variable *var);
void       widget_combobox_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_COMBOBOX_H */
