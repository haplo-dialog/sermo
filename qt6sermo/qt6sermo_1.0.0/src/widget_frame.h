/*
 * widget_frame.h — Cadre avec label Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_FRAME_H
#define WIDGET_FRAME_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_frame_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_frame_envvar_construct(GtkWidget *widget);
gchar     *widget_frame_envvar_all_construct(variable *var);
void       widget_frame_clear(variable *var);
void       widget_frame_refresh(variable *var);
void       widget_frame_fileselect(variable *var, const char *name, const char *value);
void       widget_frame_removeselected(variable *var);
void       widget_frame_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_FRAME_H */
