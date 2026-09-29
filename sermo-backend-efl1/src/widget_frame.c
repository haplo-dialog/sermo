/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_frame.c — Cadre EFL (elm_frame)
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
#include "widget_frame.h"
#include "widget_vbox.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_frame_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *fr = elm_frame_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));

    /* Titre : attribut de balise <frame label="..."> d'abord (forme des
     * exemples publics), repli sur l'element <label>. */
    {
        const char *tv = attr ? get_tag_attribute(attr, "label") : NULL;
        if (tv && *tv) {
            elm_object_text_set(fr, tv);
        } else if (Attr) {
            GList *el = NULL;
            gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
            if (lbl && *lbl) elm_object_text_set(fr, lbl);
        }
    }

    stackelement s = pop();
    Evas_Object *contenu = (Evas_Object *)s.widgets[0];
    if (s.nwidgets > 1) {
        /* L'étalon range TOUS les enfants d'un cadre dans une boîte verticale
         * (sermo-backend-gtk3/src/widget_frame.c). Jusqu'à la 2.6.8 ce port ne
         * gardait que le premier : les suivants étaient créés mais jamais
         * affichés. La <vbox> du port les empile, avec ses règles de taille. */
        push(s);
        contenu = (Evas_Object *)widget_vbox_create(NULL, NULL, 0);
    }
    if (contenu) {
        elm_object_content_set(fr, contenu);
        evas_object_show(contenu);
    }
    evas_object_show(fr);
    return (GtkWidget *)fr;
}

/* L'étalon exporte le TITRE du cadre (gtk_frame_get_label) ; un cadre sans
 * titre rend une chaîne vide. Ce port rendait TOUJOURS vide — le cas 38 du banc
 * de comportement l'a mesuré. Un conteneur qui n'exporte rien, ça se décrète ;
 * ici l'étalon exporte, donc on exporte. */
gchar *widget_frame_envvar_construct(GtkWidget *w)
{
    const char *t = w ? elm_object_text_get((Evas_Object *)w) : NULL;
    return g_strdup(t ? t : "");
}
gchar *widget_frame_envvar_all_construct(variable *var) { return NULL; }
void   widget_frame_clear(variable *var) {}
void   widget_frame_refresh(variable *var) {}
void   widget_frame_fileselect(variable *var, const char *n, const char *v) {}
void   widget_frame_removeselected(variable *var) {}
void   widget_frame_save(variable *var) {}
