/*
 * widget_vbox.h — Conteneur vertical Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_VBOX_H
#define WIDGET_VBOX_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_vbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_vbox_envvar_construct(GtkWidget *widget);
gchar     *widget_vbox_envvar_all_construct(variable *var);
void       widget_vbox_clear(variable *var);
void       widget_vbox_refresh(variable *var);
void       widget_vbox_fileselect(variable *var, const char *name, const char *value);
void       widget_vbox_removeselected(variable *var);
void       widget_vbox_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_VBOX_H */
