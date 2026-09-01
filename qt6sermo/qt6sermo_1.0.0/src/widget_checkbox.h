#ifndef WIDGET_CHECKBOX_H
#define WIDGET_CHECKBOX_H
#include "gtk3d.h"
#include "attributes.h"
#include "tag_attributes.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar *widget_checkbox_envvar_construct(GtkWidget *widget);
gchar *widget_checkbox_envvar_all_construct(variable *var);
void   widget_checkbox_clear(variable *var);
void   widget_checkbox_refresh(variable *var);
void   widget_checkbox_fileselect(variable *var, const char *n, const char *v);
void   widget_checkbox_removeselected(variable *var);
void   widget_checkbox_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
