#ifndef WIDGET_ENTRY_H
#define WIDGET_ENTRY_H
#include "gtk3d.h"
#include "attributes.h"
#include "tag_attributes.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_entry_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar *widget_entry_envvar_construct(GtkWidget *widget);
gchar *widget_entry_envvar_all_construct(variable *var);
void   widget_entry_clear(variable *var);
void   widget_entry_refresh(variable *var);
void   widget_entry_fileselect(variable *var, const char *n, const char *v);
void   widget_entry_removeselected(variable *var);
void   widget_entry_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
