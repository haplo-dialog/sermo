/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_vbox.c — Conteneur vertical EFL (elm_box vertical)
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
#include "widget_vbox.h"
#include <stdlib.h>
#include <string.h>

GtkWidget *widget_vbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *box = elm_box_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    elm_box_horizontal_set(box, EINA_FALSE);
    elm_box_align_set(box, 0.5, 0.0);   /* contenu ancre en HAUT (etalon), pas centre */

    if (attr) {
        const char *se = get_tag_attribute(attr, "space-expand");
        if (se && (!strcasecmp(se, "false") || !strcasecmp(se, "no") || !strcmp(se, "0")))
            evas_object_data_set(box, "sermo_noexpand", (void *)1);
        const char *v = get_tag_attribute(attr, "spacing");
        if (v) elm_box_padding_set(box, 0, atoi(v));
    }

    /* Le conteneur récupère UN seul élément de pile contenant tous ses
     * enfants (coalescés par l'instruction SUM de l'automate), exactement
     * comme le port GTK de référence. Un pop() par enfant provoquerait un
     * « stack underflow » faute de sentinelle. widgets[0] est le premier
     * enfant déclaré ; pack_end conserve donc l'ordre du document. */
    stackelement s = pop();
    int n;
    for (n = 0; n < s.nwidgets; ++n) {
        Evas_Object *c = (Evas_Object *)s.widgets[n];
        /* Sans hints, elm centre chaque enfant a sa taille minimale :
         * l'entry devenait invisible (largeur ~0). Remplir en largeur. */
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
            /* un enfant qui demande a s'etendre (edit, listes) garde sa
             * hauteur extensible ; les autres restent a leur taille */
            evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, wy > 0 ? EVAS_HINT_EXPAND : 0.0);
            evas_object_size_hint_align_set(c, EVAS_HINT_FILL, wy > 0 ? EVAS_HINT_FILL : 0.5);
        }
        elm_box_pack_end(box, c);
        evas_object_show(c);
    }
    evas_object_show(box);
    return (GtkWidget *)box;
}

gchar *widget_vbox_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_vbox_envvar_all_construct(variable *var) { return NULL; }
void   widget_vbox_clear(variable *var) {}
void   widget_vbox_refresh(variable *var) {}
void   widget_vbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_vbox_removeselected(variable *var) {}
void   widget_vbox_save(variable *var) {}
