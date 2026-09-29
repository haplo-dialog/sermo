/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef WIDGET_SEARCHENTRY_H
#define WIDGET_SEARCHENTRY_H
#ifndef QT6_COMPAT_H
#include <gtk/gtk.h>   /* variante gtk : vrais types ; variante neutre : shim déjà inclus */
#endif
#include "attributes.h"
#include "tag_attributes.h"
#include "variables.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_searchentry_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_searchentry_envvar_construct(GtkWidget *w);
gchar     *widget_searchentry_envvar_all_construct(variable *v);
void       widget_searchentry_clear(variable *v);
void       widget_searchentry_refresh(variable *v);
void       widget_searchentry_fileselect(variable *v, const char *n, const char *val);
void       widget_searchentry_removeselected(variable *v);
void       widget_searchentry_save(variable *v);
#ifdef __cplusplus
}
#endif
#endif /* WIDGET_SEARCHENTRY_H */
