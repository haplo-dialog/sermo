/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_edit.c — Zone de texte éditable EFL
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
#include "widget_edit.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Pose un texte BRUT : elm_entry ne connaît que le balisage EFL. Posé tel
 * quel, un saut de ligne disparaissait dès que l'entry passait dans son
 * enveloppe défilante (mesuré : « a\nb\n » ressortait « ab »), et
 * « < » ou « & » étaient pris pour du balisage. */
static void _edit_set_plain(Evas_Object *en, const char *text)
{
    char *markup = elm_entry_utf8_to_markup(text);
    elm_entry_entry_set(en, markup ? markup : "");
    free(markup);
}

GtkWidget *widget_edit_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *en = elm_entry_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    elm_entry_single_line_set(en, EINA_FALSE);
    elm_entry_editable_set(en, EINA_TRUE);
    elm_entry_scrollable_set(en, EINA_TRUE);
    evas_object_size_hint_min_set(en, 200, 100);
    evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);

    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) _edit_set_plain(en, def);
        /* <input> : lu par widget_edit_refresh(), que le cœur appelle juste
         * après la création — une action refresh le relit ainsi elle aussi. */
    }
    /* background / foreground (extension sermo) : style texte utilisateur ;
     * le fond est pose sur le rectangle de gabarit de l'enveloppe (voir
     * efl_container_add : data "sermo_bg"). */
    if (attr) {
        const char *bg = get_tag_attribute(attr, "background");
        const char *fg = get_tag_attribute(attr, "foreground");
        if (fg && *fg) {
            char st[96]; snprintf(st, sizeof(st), "DEFAULT='color=%s'", fg);
            elm_entry_text_style_user_push(en, st);
        }
        if (bg && *bg) {
            evas_object_data_set(en, "sermo_bg", strdup(bg));
            /* l'entry « scrollable » peint son propre fond de theme par-dessus
             * le rectangle : sans, elle est transparente et le fond se voit */
            elm_entry_scrollable_set(en, EINA_FALSE);
        }
    }
    evas_object_show(en);

    return (GtkWidget *)en;
}

static Evas_Object *edit_entry(GtkWidget *w)
{
    Evas_Object *en = w ? (Evas_Object *)evas_object_data_get((Evas_Object *)w, "entry") : NULL;
    return en ? en : (Evas_Object *)w;
}

gchar *widget_edit_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("");
    const char *txt = elm_entry_entry_get(edit_entry(widget));
    /* l'entry rend du balisage (<br/>, &amp;…) : on exporte le texte brut */
    char *plain = txt ? elm_entry_markup_to_utf8(txt) : NULL;
    gchar *value = g_strdup(plain ? plain : "");
    free(plain);
    return value;
}
gchar *widget_edit_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_edit_envvar_construct(var->Widget);
}
void widget_edit_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_entry_entry_set(edit_entry(var->Widget), "");
}
void widget_edit_refresh(variable *var)
{
    gchar *text;

    if (!var || !var->Widget || !var->Attributes) return;
    /* <input> (commande ou fichier) : le contenu ENTIER, saut de ligne final
     * compris — règle de l'étalon gtk3sermo. */
    text = widget_input_text(var->Attributes);
    if (text) {
        _edit_set_plain(edit_entry(var->Widget), text);
        g_free(text);
    }
}
void widget_edit_fileselect(variable *var, const char *n, const char *v) {}
void widget_edit_removeselected(variable *var) {}
void widget_edit_save(variable *var) {}
