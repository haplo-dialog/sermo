/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_eventbox.c — Conteneur capteur d'évènements EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <eventbox> est un CONTENEUR : il emballe un enfant pour lui donner une zone
 * cliquable. Le stub précédent ne dépilait pas la pile — l'enfant était PERDU.
 * Ici le nœud empile ses enfants dans une elm_box, exactement comme <vbox>, et
 * capte le clic (EVAS_CALLBACK_MOUSE_DOWN) pour jouer l'<action>.
 *
 * Export : chaîne VIDE, comme l'étalon gtk3.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "actions.h"
#include "widget_eventbox.h"
#include <stdlib.h>

/* Toutes les <action> de la zone, via le répartiteur du cœur. */
static void _eventbox_clic(void *data, Evas *e, Evas_Object *obj, void *ev)
{
    AttributeSet *Attr = (AttributeSet *) data;
    GList *el = NULL;
    gchar *fn;
    (void) e; (void) ev;
    if (!Attr) return;
    fn = attributeset_get_first(&el, Attr, ATTR_ACTION);
    while (fn) {
        if (*fn) execute_action((GtkWidget *) obj, fn, NULL);
        fn = attributeset_get_next(&el, Attr, ATTR_ACTION);
    }
}

GtkWidget *widget_eventbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) attr; (void) Type;
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *box = elm_box_add(parent ? parent
                                          : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
    elm_box_horizontal_set(box, EINA_FALSE);
    elm_box_align_set(box, 0.5, 0.0);

    /* Un seul pop : le cœur coalesce les enfants (instruction SUM). */
    stackelement s = pop();
    int n;
    for (n = 0; n < s.nwidgets; ++n) {
        Evas_Object *c = (Evas_Object *) s.widgets[n];
        if (!c) continue;
        evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(c, EVAS_HINT_FILL, 0.5);
        elm_box_pack_end(box, c);
        evas_object_show(c);
    }

    if (Attr && attributeset_is_avail(Attr, ATTR_ACTION))
        evas_object_event_callback_add(box, EVAS_CALLBACK_MOUSE_DOWN,
                                       _eventbox_clic, Attr);
    evas_object_show(box);
    return (GtkWidget *) box;
}

gchar *widget_eventbox_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");            /* étalon gtk3 : valeur vide */
}
gchar *widget_eventbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_eventbox_envvar_construct(var->Widget);
}
void widget_eventbox_clear(variable *var)          { (void) var; }
void widget_eventbox_refresh(variable *var)        { (void) var; }
void widget_eventbox_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_eventbox_removeselected(variable *var) { (void) var; }
void widget_eventbox_save(variable *var)           { (void) var; }
