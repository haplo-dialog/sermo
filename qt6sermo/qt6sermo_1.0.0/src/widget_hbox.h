/*
 * widget_hbox.h — Conteneur horizontal Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_HBOX_H
#define WIDGET_HBOX_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_hbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_hbox_envvar_construct(GtkWidget *widget);
gchar     *widget_hbox_envvar_all_construct(variable *var);
void       widget_hbox_clear(variable *var);
void       widget_hbox_refresh(variable *var);
void       widget_hbox_fileselect(variable *var, const char *name, const char *value);
void       widget_hbox_removeselected(variable *var);
void       widget_hbox_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_HBOX_H */
