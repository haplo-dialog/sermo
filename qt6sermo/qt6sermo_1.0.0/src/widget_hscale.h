/*
 * widget_hscale.h — Glissière horizontale Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_HSCALE_H
#define WIDGET_HSCALE_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_hscale_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_hscale_envvar_construct(GtkWidget *widget);
gchar     *widget_hscale_envvar_all_construct(variable *var);
void       widget_hscale_clear(variable *var);
void       widget_hscale_refresh(variable *var);
void       widget_hscale_fileselect(variable *var, const char *name, const char *value);
void       widget_hscale_removeselected(variable *var);
void       widget_hscale_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_HSCALE_H */
