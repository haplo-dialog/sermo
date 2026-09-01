#ifndef WIDGET_INFOBAR_H
#define WIDGET_INFOBAR_H
#include "qt6-compat.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "variables.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_infobar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_infobar_envvar_construct(GtkWidget *w);
gchar     *widget_infobar_envvar_all_construct(variable *v);
void       widget_infobar_clear(variable *v);
void       widget_infobar_refresh(variable *v);
void       widget_infobar_fileselect(variable *v, const char *n, const char *val);
void       widget_infobar_removeselected(variable *v);
void       widget_infobar_save(variable *v);
#ifdef __cplusplus
}
#endif
#endif /* WIDGET_INFOBAR_H */
