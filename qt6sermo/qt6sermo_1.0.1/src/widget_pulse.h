/*
 * widget_pulse.h — stub Qt6 (widget non encore porté)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le tag <pulse> est reconnu par le parser mais n'a pas d'implémentation
 * native Qt6. Ces fonctions sont des stubs qui évitent l'échec de link
 * et avertissent l'utilisateur à l'exécution.
 */
#ifndef WIDGET_PULSE_H
#define WIDGET_PULSE_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_pulse_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_pulse_envvar_construct(GtkWidget *widget);
gchar     *widget_pulse_envvar_all_construct(variable *var);
void       widget_pulse_clear(variable *var);
void       widget_pulse_refresh(variable *var);
void       widget_pulse_fileselect(variable *var, const char *name, const char *value);
void       widget_pulse_removeselected(variable *var);
void       widget_pulse_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_PULSE_H */
