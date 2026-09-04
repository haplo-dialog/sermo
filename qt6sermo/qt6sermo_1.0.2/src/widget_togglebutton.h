#ifndef WIDGET_TOGGLEBUTTON_H
#define WIDGET_TOGGLEBUTTON_H
#include "gtk3d.h"
#include "attributes.h"
#include "tag_attributes.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_togglebutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar *widget_togglebutton_envvar_construct(GtkWidget *widget);
gchar *widget_togglebutton_envvar_all_construct(variable *var);
void   widget_togglebutton_clear(variable *var);
void   widget_togglebutton_refresh(variable *var);
void   widget_togglebutton_fileselect(variable *var, const char *n, const char *v);
void   widget_togglebutton_removeselected(variable *var);
void   widget_togglebutton_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
