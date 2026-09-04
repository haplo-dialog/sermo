/*
 * widget_terminal.h — Terminal embarqué Qt6 (QTermWidget optionnel)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_TERMINAL_H
#define WIDGET_TERMINAL_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_terminal_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_terminal_envvar_construct(GtkWidget *widget);
gchar     *widget_terminal_envvar_all_construct(variable *var);
void       widget_terminal_clear(variable *var);
void       widget_terminal_refresh(variable *var);
void       widget_terminal_fileselect(variable *var, const char *name, const char *value);
void       widget_terminal_removeselected(variable *var);
void       widget_terminal_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_TERMINAL_H */
