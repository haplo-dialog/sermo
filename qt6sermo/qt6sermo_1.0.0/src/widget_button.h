#ifndef WIDGET_BUTTON_H
#define WIDGET_BUTTON_H
#include "gtk3d.h"
#include "attributes.h"
#include "tag_attributes.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_button_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar *widget_button_envvar_construct(GtkWidget *widget);
gchar *widget_button_envvar_all_construct(variable *var);
void   widget_button_clear(variable *var);
void   widget_button_refresh(variable *var);
void   widget_button_fileselect(variable *var, const char *n, const char *v);
void   widget_button_removeselected(variable *var);
void   widget_button_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
