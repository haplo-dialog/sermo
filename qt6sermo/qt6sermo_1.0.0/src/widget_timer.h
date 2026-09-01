/*
 * widget_timer.h — Minuterie Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_TIMER_H
#define WIDGET_TIMER_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_timer_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_timer_envvar_construct(GtkWidget *widget);
gchar     *widget_timer_envvar_all_construct(variable *var);
void       widget_timer_clear(variable *var);
void       widget_timer_refresh(variable *var);
void       widget_timer_fileselect(variable *var, const char *name, const char *value);
void       widget_timer_removeselected(variable *var);
void       widget_timer_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_TIMER_H */
