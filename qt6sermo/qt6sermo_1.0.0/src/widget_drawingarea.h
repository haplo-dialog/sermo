#ifndef WIDGET_DRAWINGAREA_H
#define WIDGET_DRAWINGAREA_H
#include "qt6-compat.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "variables.h"
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *widget_drawingarea_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_drawingarea_envvar_construct(GtkWidget *w);
gchar     *widget_drawingarea_envvar_all_construct(variable *v);
void       widget_drawingarea_clear(variable *v);
void       widget_drawingarea_refresh(variable *v);
void       widget_drawingarea_fileselect(variable *v, const char *n, const char *val);
void       widget_drawingarea_removeselected(variable *v);
void       widget_drawingarea_save(variable *v);
#ifdef __cplusplus
}
#endif
#endif /* WIDGET_DRAWINGAREA_H */
