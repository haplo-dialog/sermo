#ifndef WIDGET_SPINNER_H
#define WIDGET_SPINNER_H
#include "qt6-compat.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "variables.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_spinner_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_spinner_envvar_construct(GtkWidget *w);
gchar     *widget_spinner_envvar_all_construct(variable *v);
void       widget_spinner_clear(variable *v);
void       widget_spinner_refresh(variable *v);
void       widget_spinner_fileselect(variable *v, const char *n, const char *val);
void       widget_spinner_removeselected(variable *v);
void       widget_spinner_save(variable *v);
#ifdef __cplusplus
}
#endif
#endif /* WIDGET_SPINNER_H */
