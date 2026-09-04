#ifndef WIDGET_SWITCH_H
#define WIDGET_SWITCH_H
#ifdef __cplusplus
extern "C" {
#endif
#include "qt6-compat.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "variables.h"
GtkWidget *widget_switch_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_switch_envvar_construct(GtkWidget *w);
gchar     *widget_switch_envvar_all_construct(variable *v);
void       widget_switch_clear(variable *v);
void       widget_switch_refresh(variable *v);
void       widget_switch_fileselect(variable *v, const char *n, const char *val);
void       widget_switch_removeselected(variable *v);
void       widget_switch_save(variable *v);
#ifdef __cplusplus
}
#endif
#endif
