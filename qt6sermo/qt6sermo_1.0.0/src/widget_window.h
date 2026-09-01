/*
 * widget_window.h — Fenêtre principale Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_WINDOW_H
#define WIDGET_WINDOW_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_window_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_window_envvar_construct(GtkWidget *widget);
gchar     *widget_window_envvar_all_construct(variable *var);
void       widget_window_clear(variable *var);
void       widget_window_refresh(variable *var);
void       widget_window_fileselect(variable *var, const char *name, const char *value);
void       widget_window_removeselected(variable *var);
void       widget_window_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_WINDOW_H */
