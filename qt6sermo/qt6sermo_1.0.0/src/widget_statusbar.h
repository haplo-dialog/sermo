/*
 * widget_statusbar.h — Barre de statut Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_STATUSBAR_H
#define WIDGET_STATUSBAR_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_statusbar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_statusbar_envvar_construct(GtkWidget *widget);
gchar     *widget_statusbar_envvar_all_construct(variable *var);
void       widget_statusbar_clear(variable *var);
void       widget_statusbar_refresh(variable *var);
void       widget_statusbar_fileselect(variable *var, const char *name, const char *value);
void       widget_statusbar_removeselected(variable *var);
void       widget_statusbar_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_STATUSBAR_H */
