/*
 * widget_vscale.h — GtkScale vertical (GTK4)
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_VSCALE_H
#define WIDGET_VSCALE_H

void       widget_vscale_clear(variable *var);
GtkWidget *widget_vscale_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_vscale_envvar_all_construct(variable *var);
gchar     *widget_vscale_envvar_construct(GtkWidget *widget);
void       widget_vscale_fileselect(variable *var, const char *name, const char *value);
void       widget_vscale_refresh(variable *var);
void       widget_vscale_removeselected(variable *var);
void       widget_vscale_save(variable *var);

#endif
