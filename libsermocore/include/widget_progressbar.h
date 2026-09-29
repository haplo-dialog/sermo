/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_progressbar.h — Barre de progression Qt6
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_PROGRESSBAR_H
#define WIDGET_PROGRESSBAR_H

#ifndef QT6_COMPAT_H
#include <gtk/gtk.h>   /* variante gtk : vrais types ; variante neutre : shim déjà inclus */
#endif
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *widget_progressbar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_progressbar_envvar_construct(GtkWidget *widget);
gchar     *widget_progressbar_envvar_all_construct(variable *var);
void       widget_progressbar_clear(variable *var);
void       widget_progressbar_refresh(variable *var);
void       widget_progressbar_fileselect(variable *var, const char *name, const char *value);
void       widget_progressbar_removeselected(variable *var);
void       widget_progressbar_save(variable *var);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_PROGRESSBAR_H */
