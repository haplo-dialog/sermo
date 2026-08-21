/*
 * widget_overlay.h — GTK4-native widget (overlay)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_OVERLAY_H
#define WIDGET_OVERLAY_H

void       widget_overlay_clear(variable *var);
GtkWidget *widget_overlay_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_overlay_envvar_all_construct(variable *var);
gchar     *widget_overlay_envvar_construct(GtkWidget *widget);
void       widget_overlay_fileselect(variable *var, const char *name, const char *value);
void       widget_overlay_refresh(variable *var);
void       widget_overlay_removeselected(variable *var);
void       widget_overlay_save(variable *var);

#endif
