#ifndef WIDGET_RADIOBUTTON_H
#define WIDGET_RADIOBUTTON_H
#include "gtk3d.h"
#include "attributes.h"
#include "tag_attributes.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_radiobutton_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar *widget_radiobutton_envvar_construct(GtkWidget *widget);
gchar *widget_radiobutton_envvar_all_construct(variable *var);
void   widget_radiobutton_clear(variable *var);
void   widget_radiobutton_refresh(variable *var);
void   widget_radiobutton_fileselect(variable *var, const char *n, const char *v);
void   widget_radiobutton_removeselected(variable *var);
void   widget_radiobutton_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
