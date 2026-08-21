/*
 * widget_vseparator.h — GtkSeparator vertical (GTK4)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_VSEPARATOR_H
#define WIDGET_VSEPARATOR_H

void       widget_vseparator_clear(variable *var);
GtkWidget *widget_vseparator_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_vseparator_envvar_all_construct(variable *var);
gchar     *widget_vseparator_envvar_construct(GtkWidget *widget);
void       widget_vseparator_fileselect(variable *var, const char *name, const char *value);
void       widget_vseparator_refresh(variable *var);
void       widget_vseparator_removeselected(variable *var);
void       widget_vseparator_save(variable *var);

#endif
