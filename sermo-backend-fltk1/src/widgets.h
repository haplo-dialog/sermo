/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widgets.h — Dispatch des widgets FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Adapté de gtk3d/widgets.h (László Pere, Thunor)
 */

#ifndef WIDGETS_H
#define WIDGETS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
/* FLTK port: pas de <gtk/gtk.h> — fltk-compat.h est force-inclus */
#include "gtkdialog.h"
#include "stack.h"
#include "attributes.h"
#include "stringman.h"
#include "variables.h"
#include "automaton.h"

/* Listes de visibilité différée — définies dans widgets.cpp (C++ linkage) */
extern GList *widget_show_list;
extern GList *widget_hide_list;

#ifdef __cplusplus
extern "C" {
#endif

extern int SERMO_FLEX_TAG;   /* user_data() des conteneurs Fl_Flex du port */
char    *widget_get_text_value(GtkWidget *widget, int type);
FILE    *widget_opencommand(const char *command);
gchar   *widget_input_text(AttributeSet *Attr);
/* Lignes du texte de widget_input_text(), sans CR/LF ; g_strfreev ; NULL si rien. */
gchar  **widget_input_lines(AttributeSet *Attr);
char    *widgets_to_str(int itype);
gboolean widget_connect_signals(GtkWidget *widget, AttributeSet *Attr);
void     widget_visibility_list_add(GtkWidget *widget, tag_attr *attr);
void     widget_show_all(void);


#ifdef __cplusplus
}
bool sermo_widget_expands(Fl_Widget *w);   /* C++ seulement */
bool sermo_widget_noexpand(Fl_Widget *w);
#endif

#endif /* WIDGETS_H */
