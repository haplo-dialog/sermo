/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hbox.c — Conteneur horizontal EFL (elm_box horizontal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_hbox.h"
#include "widget_vbox.h"
#include <stdlib.h>
#include <string.h>

GtkWidget *widget_hbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *box = elm_box_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    elm_box_horizontal_set(box, EINA_TRUE);
    /* calage (gauche / droite) : posé par efl_boite_ranger */

    if (attr) {
        const char *v = get_tag_attribute(attr, "spacing");
        if (v) elm_box_padding_set(box, atoi(v), 0);
    }

    /* Le conteneur récupère UN seul élément de pile contenant tous ses
     * enfants (coalescés par l'instruction SUM de l'automate), comme le
     * port GTK de référence. Un pop() par enfant provoquerait un
     * « stack underflow » faute de sentinelle. widgets[0] est le premier
     * enfant déclaré ; pack_end conserve donc l'ordre du document.
     * Étirement, remplissage, homogeneous et calage à droite : règle de
     * l'étalon, partagée avec la colonne (efl_boite_ranger, widget_vbox.c). */
    stackelement s = pop();
    efl_boite_ranger(box, &s, 1, attr);
    evas_object_show(box);
    return (GtkWidget *)box;
}

gchar *widget_hbox_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_hbox_envvar_all_construct(variable *var) { return NULL; }
void   widget_hbox_clear(variable *var) {}
void   widget_hbox_refresh(variable *var) {}
void   widget_hbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_hbox_removeselected(variable *var) {}
void   widget_hbox_save(variable *var) {}
