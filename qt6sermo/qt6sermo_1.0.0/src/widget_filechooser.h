/*
 * widget_filechooser.h — stub Qt6 (widget non encore porté)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le tag <filechooser> est reconnu par le parser mais n'a pas d'implémentation
 * native Qt6. Ces fonctions sont des stubs qui évitent l'échec de link
 * et avertissent l'utilisateur à l'exécution.
 */
#ifndef WIDGET_FILECHOOSER_H
#define WIDGET_FILECHOOSER_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_filechooser_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_filechooser_envvar_construct(GtkWidget *widget);
gchar     *widget_filechooser_envvar_all_construct(variable *var);
void       widget_filechooser_clear(variable *var);
void       widget_filechooser_refresh(variable *var);
void       widget_filechooser_fileselect(variable *var, const char *name, const char *value);
void       widget_filechooser_removeselected(variable *var);
void       widget_filechooser_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_FILECHOOSER_H */
