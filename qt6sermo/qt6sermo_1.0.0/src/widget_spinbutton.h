/*
 * widget_spinbutton.h — Champ numérique Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_SPINBUTTON_H
#define WIDGET_SPINBUTTON_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_spinbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_spinbutton_envvar_construct(GtkWidget *widget);
gchar     *widget_spinbutton_envvar_all_construct(variable *var);
void       widget_spinbutton_clear(variable *var);
void       widget_spinbutton_refresh(variable *var);
void       widget_spinbutton_fileselect(variable *var, const char *name, const char *value);
void       widget_spinbutton_removeselected(variable *var);
void       widget_spinbutton_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_SPINBUTTON_H */
