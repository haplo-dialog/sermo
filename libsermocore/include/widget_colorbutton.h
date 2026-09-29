/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_colorbutton.h — Sélecteur de couleur Qt6
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_COLORBUTTON_H
#define WIDGET_COLORBUTTON_H

#ifndef QT6_COMPAT_H
#include <gtk/gtk.h>   /* variante gtk : vrais types ; variante neutre : shim déjà inclus */
#endif
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_colorbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_colorbutton_envvar_construct(GtkWidget *widget);
gchar     *widget_colorbutton_envvar_all_construct(variable *var);
void       widget_colorbutton_clear(variable *var);
void       widget_colorbutton_refresh(variable *var);
void       widget_colorbutton_fileselect(variable *var, const char *name, const char *value);
void       widget_colorbutton_removeselected(variable *var);
void       widget_colorbutton_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_COLORBUTTON_H */
