/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_drawingarea.h — GTK4-native widget (drawingarea)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_DRAWINGAREA_H
#define WIDGET_DRAWINGAREA_H

void       widget_drawingarea_clear(variable *var);
GtkWidget *widget_drawingarea_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_drawingarea_envvar_all_construct(variable *var);
gchar     *widget_drawingarea_envvar_construct(GtkWidget *widget);
void       widget_drawingarea_fileselect(variable *var, const char *name, const char *value);
void       widget_drawingarea_refresh(variable *var);
void       widget_drawingarea_removeselected(variable *var);
void       widget_drawingarea_save(variable *var);

#endif
