/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_menu.c — <menu> EFL : dépile ses <menuitem> et pousse un carrier de
 * menu (label + items) que <menubar> consomme. GPL-2.0-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "automaton.h"
#include "attributes.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_menu.h"
#include "efl_menu_model.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    EMenu *m = calloc(1, sizeof(EMenu));
    if (attr) {
        const char *tv = get_tag_attribute(attr, "label");
        if (tv && *tv) m->label = strdup(tv);
    }
    if (!m->label && Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) m->label = strdup(lbl);
    }

    stackelement s = pop();
    if (s.nwidgets > 0) m->items = calloc(s.nwidgets, sizeof(EMenuItem));
    for (int n = 0; n < s.nwidgets; n++) {
        Evas_Object *w = (Evas_Object *)s.widgets[n];
        if (!w) continue;
        EMenuItem *mi = (EMenuItem *)evas_object_data_get(w, "emi");
        if (mi) m->items[m->n++] = *mi;   /* copie superficielle : le carrier ne libère pas */
    }

    Evas_Object *win = efl_main_win_get();
    Evas_Object *carrier = evas_object_rectangle_add(evas_object_evas_get(win));
    evas_object_data_set(carrier, "emenu", m);
    return (GtkWidget *)carrier;
}

gchar *widget_menu_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_menu_envvar_all_construct(variable *var) { return NULL; }
void   widget_menu_clear(variable *var) {}
void   widget_menu_refresh(variable *var) {}
void   widget_menu_fileselect(variable *var, const char *n, const char *v) {}
void   widget_menu_removeselected(variable *var) {}
void   widget_menu_save(variable *var) {}
