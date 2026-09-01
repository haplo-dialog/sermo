/*
 * widget_comboboxtext.h — ComboBox éditable Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_COMBOBOXTEXT_H
#define WIDGET_COMBOBOXTEXT_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_comboboxtext_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_comboboxtext_envvar_construct(GtkWidget *widget);
gchar     *widget_comboboxtext_envvar_all_construct(variable *var);
void       widget_comboboxtext_clear(variable *var);
void       widget_comboboxtext_refresh(variable *var);
void       widget_comboboxtext_fileselect(variable *var, const char *name, const char *value);
void       widget_comboboxtext_removeselected(variable *var);
void       widget_comboboxtext_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_COMBOBOXTEXT_H */
