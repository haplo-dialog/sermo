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
#include <stdlib.h>
#include <string.h>

GtkWidget *widget_hbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *box = elm_box_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    elm_box_horizontal_set(box, EINA_TRUE);
    elm_box_align_set(box, 0.0, 0.5);   /* contenu ancre a GAUCHE */

    if (attr) {
        const char *se = get_tag_attribute(attr, "space-expand");
        if (se && (!strcasecmp(se, "false") || !strcasecmp(se, "no") || !strcmp(se, "0")))
            evas_object_data_set(box, "sermo_noexpand", (void *)1);
        const char *v = get_tag_attribute(attr, "spacing");
        if (v) elm_box_padding_set(box, atoi(v), 0);
    }

    /* Le conteneur récupère UN seul élément de pile contenant tous ses
     * enfants (coalescés par l'instruction SUM de l'automate), comme le
     * port GTK de référence. Un pop() par enfant provoquerait un
     * « stack underflow » faute de sentinelle. widgets[0] est le premier
     * enfant déclaré ; pack_end conserve donc l'ordre du document. */
    stackelement s = pop();
    int n;
    /* gtkdialog empile par pack_end : sans enfant extensible, la rangee
     * est calee a DROITE a sa taille naturelle (etalon gtk3) */
    int any_expand = 0;
    for (n = 0; n < s.nwidgets; ++n) {
        Evas_Object *c = (Evas_Object *)s.widgets[n];
        const char *ty = evas_object_type_get(c);
        if (evas_object_data_get(c, "sermo_expand")) any_expand = 1;
        else if (!evas_object_data_get(c, "sermo_noexpand") && ty &&
                 (strstr(ty, "entry") || strstr(ty, "frame") || strstr(ty, "box") ||
                  strstr(ty, "panes") ||
                  strstr(ty, "genlist") || strstr(ty, "table") || evas_object_data_get(c, "sw_rect")))
            any_expand = 1;
    }
    if (!any_expand) {
        elm_box_align_set(box, 1.0, 0.5);
        for (n = 0; n < s.nwidgets; ++n) {
            Evas_Object *c = (Evas_Object *)s.widgets[n];
            evas_object_size_hint_weight_set(c, 0.0, 0.0);
            evas_object_size_hint_align_set(c, 0.5, 0.5);
            elm_box_pack_end(box, c);
            evas_object_show(c);
        }
        evas_object_show(box);
        return (GtkWidget *)box;
    }
    for (n = 0; n < s.nwidgets; ++n) {
        {
            Evas_Object *c = (Evas_Object *)s.widgets[n];
            /* hbox : l'expansion utile est HORIZONTALE (les axes etaient
             * inverses — les entries se retrouvaient a largeur nulle) */
            {
                double wx = 0, wy = 0;
                evas_object_size_hint_weight_get(c, &wx, &wy);
            {
                /* conteneur (box, frame, table = enveloppe defilante) :
                 * il remplit sa cellule, comme GTK avec space-expand */
                const char *ty = evas_object_type_get(c);
                if ((ty && (strstr(ty, "box") || strstr(ty, "frame") || strstr(ty, "table") ||
                            strstr(ty, "panes"))   /* <paned> : une poignée sans
                                                    * place à partager ne sert à rien */) ||
                    evas_object_data_get(c, "sw_rect"))
                    wy = 1.0;
                if (evas_object_data_get(c, "sermo_noexpand")) wy = 0.0;
            }
                if (evas_object_data_get(c, "sermo_noexpand")) {
                    /* colonne fixe (logo) : largeur naturelle, collee a gauche */
                    evas_object_size_hint_weight_set(c, 0.0, EVAS_HINT_EXPAND);
                    evas_object_size_hint_align_set(c, 0.0, EVAS_HINT_FILL);
                } else {
                evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, wy > 0 ? EVAS_HINT_EXPAND : 0.0);
                evas_object_size_hint_align_set(c, EVAS_HINT_FILL, wy > 0 ? EVAS_HINT_FILL : 0.5);
                }
            }
            elm_box_pack_end(box, c);
            evas_object_show(c);
        }
    }
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
