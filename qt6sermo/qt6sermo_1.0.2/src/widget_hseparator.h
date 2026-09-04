/*
 * widget_hseparator.h — Séparateur horizontal Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_HSEPARATOR_H
#define WIDGET_HSEPARATOR_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_hseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_hseparator_envvar_construct(GtkWidget *widget);
gchar     *widget_hseparator_envvar_all_construct(variable *var);
void       widget_hseparator_clear(variable *var);
void       widget_hseparator_refresh(variable *var);
void       widget_hseparator_fileselect(variable *var, const char *name, const char *value);
void       widget_hseparator_removeselected(variable *var);
void       widget_hseparator_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_HSEPARATOR_H */
