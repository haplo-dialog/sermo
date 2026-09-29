/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* SEARCHENTRY.h — Widget <searchentry> ncurses (terminal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef WIDGET_SEARCHENTRY_H
#define WIDGET_SEARCHENTRY_H
#include "ncurses-compat.h"
#include "dialog_state.h"
#include "variables.h"
#include "attributes.h"
#include "tag_attributes.h"
GtkWidget *widget_searchentry_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_searchentry_envvar_construct(GtkWidget *widget);
gchar     *widget_searchentry_envvar_all_construct(variable *var);
void       widget_searchentry_clear(variable *var);
void       widget_searchentry_refresh(variable *var);
void       widget_searchentry_fileselect(variable *var, const char *name, const char *value);
void       widget_searchentry_removeselected(variable *var);
void       widget_searchentry_save(variable *var);
#endif /* WIDGET_SEARCHENTRY_H */
