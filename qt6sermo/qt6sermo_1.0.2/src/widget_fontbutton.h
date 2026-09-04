/*
 * widget_fontbutton.h — Sélecteur de police Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_FONTBUTTON_H
#define WIDGET_FONTBUTTON_H

#include "qt6-compat.h"
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_fontbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_fontbutton_envvar_construct(GtkWidget *widget);
gchar     *widget_fontbutton_envvar_all_construct(variable *var);
void       widget_fontbutton_clear(variable *var);
void       widget_fontbutton_refresh(variable *var);
void       widget_fontbutton_fileselect(variable *var, const char *name, const char *value);
void       widget_fontbutton_removeselected(variable *var);
void       widget_fontbutton_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_FONTBUTTON_H */
