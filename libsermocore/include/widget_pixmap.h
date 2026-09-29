/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_pixmap.h — Image / icône Qt6
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_PIXMAP_H
#define WIDGET_PIXMAP_H

#ifndef QT6_COMPAT_H
#include <gtk/gtk.h>   /* variante gtk : vrais types ; variante neutre : shim déjà inclus */
#endif
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_pixmap_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_pixmap_envvar_construct(GtkWidget *widget);
gchar     *widget_pixmap_envvar_all_construct(variable *var);
void       widget_pixmap_clear(variable *var);
void       widget_pixmap_refresh(variable *var);
void       widget_pixmap_fileselect(variable *var, const char *name, const char *value);
void       widget_pixmap_removeselected(variable *var);
void       widget_pixmap_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_PIXMAP_H */
