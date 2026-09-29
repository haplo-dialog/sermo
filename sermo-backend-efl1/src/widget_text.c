/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_text.c — Zone de texte lecture seule EFL
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
#include "widget_text.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_text_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    /* Un <text> est une ETIQUETTE (elm_label), pas une entry non editable :
     * l'ancienne entry scrollable rendait une barre vide sans texte, et ne
     * lisait jamais <label>. Le texte vit en data "txt" pour l'export. */
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *lb = elm_label_add(parent);
    gchar *txt = NULL;
    (void)attr; (void)Type;
    /* pas de retour a la ligne par defaut (gtkdialog non plus) : en WRAP_WORD
     * la largeur minimale est 0 et l'etiquette DISPARAIT des qu'elle n'est
     * plus etiree (rangees calees a droite). wrap="true" pour l'activer. */
    {
        const char *wv = attr ? get_tag_attribute(attr, "wrap") : NULL;
        elm_label_line_wrap_set(lb, (wv && (!strcasecmp(wv, "true") || !strcmp(wv, "1")))
                                    ? ELM_WRAP_WORD : ELM_WRAP_NONE);
    }
    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) txt = g_strdup(lbl);
        if (!txt) {
            el = NULL;
            gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
            if (def && *def) txt = g_strdup(def);
        }
        /* <input> : lu par widget_text_refresh(), que le cœur appelle juste
         * après la création. */
    }
    if (txt) {
        gchar *markup = g_markup_escape_text(txt, -1);
        elm_object_text_set(lb, markup);
        g_free(markup);
        evas_object_data_set(lb, "txt", txt);
    }
    evas_object_size_hint_align_set(lb, 0.0, 0.5);
    evas_object_show(lb);
    return (GtkWidget *)lb;
}

gchar *widget_text_envvar_construct(GtkWidget *widget)
{
    const char *txt;
    if (!widget) return g_strdup("");
    txt = (const char *)evas_object_data_get((Evas_Object *)widget, "txt");
    return g_strdup(txt ? txt : "");
}
gchar *widget_text_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_text_envvar_construct(var->Widget);
}
void widget_text_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_object_text_set((Evas_Object *)var->Widget, "");
    free(evas_object_data_get((Evas_Object *)var->Widget, "txt"));
    evas_object_data_set((Evas_Object *)var->Widget, "txt", NULL);
}
void widget_text_refresh(variable *var)
{
    if (!var || !var->Widget || !var->Attributes) return;
    /* <input> (commande ou fichier) : le contenu ENTIER, saut de ligne final
     * compris — règle de l'étalon gtk3sermo (la création retirait ce saut de
     * ligne). L'export lit la donnée « txt » ; l'affichage reçoit le texte
     * échappé, comme à la création. */
    gchar *text = widget_input_text(var->Attributes);
    if (text) {
        gchar *markup = g_markup_escape_text(text, -1);
        elm_object_text_set((Evas_Object *)var->Widget, markup);
        g_free(markup);
        g_free(evas_object_data_get((Evas_Object *)var->Widget, "txt"));
        evas_object_data_set((Evas_Object *)var->Widget, "txt", text);
    }
}
void widget_text_fileselect(variable *var, const char *n, const char *v) {}
void widget_text_removeselected(variable *var) {}
void widget_text_save(variable *var) {}
