/*
 * widget_levelbar.h — GTK4-native widget (levelbar)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_LEVELBAR_H
#define WIDGET_LEVELBAR_H

void       widget_levelbar_clear(variable *var);
GtkWidget *widget_levelbar_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_levelbar_envvar_all_construct(variable *var);
gchar     *widget_levelbar_envvar_construct(GtkWidget *widget);
void       widget_levelbar_fileselect(variable *var, const char *name, const char *value);
void       widget_levelbar_refresh(variable *var);
void       widget_levelbar_removeselected(variable *var);
void       widget_levelbar_save(variable *var);

#endif
