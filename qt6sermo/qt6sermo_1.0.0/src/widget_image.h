/*
 * widget_image.h — stub Qt6 (widget non encore porté)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Le tag <image> est reconnu par le parser mais n'a pas d'implémentation
 * native Qt6. Ces fonctions sont des stubs qui évitent l'échec de link
 * et avertissent l'utilisateur à l'exécution.
 */
#ifndef WIDGET_IMAGE_H
#define WIDGET_IMAGE_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_image_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_image_envvar_construct(GtkWidget *widget);
gchar     *widget_image_envvar_all_construct(variable *var);
void       widget_image_clear(variable *var);
void       widget_image_refresh(variable *var);
void       widget_image_fileselect(variable *var, const char *name, const char *value);
void       widget_image_removeselected(variable *var);
void       widget_image_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_IMAGE_H */
