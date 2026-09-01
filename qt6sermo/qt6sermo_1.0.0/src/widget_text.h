#ifndef WIDGET_TEXT_H
#define WIDGET_TEXT_H
#include "gtk3d.h"
#include "attributes.h"
#include "tag_attributes.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_text_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar *widget_text_envvar_construct(GtkWidget *widget);
gchar *widget_text_envvar_all_construct(variable *var);
void   widget_text_clear(variable *var);
void   widget_text_refresh(variable *var);
void   widget_text_fileselect(variable *var, const char *n, const char *v);
void   widget_text_removeselected(variable *var);
void   widget_text_save(variable *var);
#ifdef __cplusplus
}
#endif
#endif
