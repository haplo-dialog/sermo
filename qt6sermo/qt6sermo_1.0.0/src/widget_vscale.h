/*
 * widget_vscale.h — Glissière verticale Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_VSCALE_H
#define WIDGET_VSCALE_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_vscale_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_vscale_envvar_construct(GtkWidget *widget);
gchar     *widget_vscale_envvar_all_construct(variable *var);
void       widget_vscale_clear(variable *var);
void       widget_vscale_refresh(variable *var);
void       widget_vscale_fileselect(variable *var, const char *name, const char *value);
void       widget_vscale_removeselected(variable *var);
void       widget_vscale_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_VSCALE_H */
