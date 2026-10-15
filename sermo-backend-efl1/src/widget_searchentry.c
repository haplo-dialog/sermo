/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_searchentry.c — Champ de recherche EFL (elm_entry + icon)
 * sermo — haplo-dialog — GPL-2.0-or-later */
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
#include "widget_searchentry.h"
#include <Elementary.h>
#include <string.h>

/* Le texte d'une elm_entry est du BALISAGE : « < » y ouvre une balise, et la
 * lecture rend « &lt; », « &laquo; »… Écrire et lire en texte brut, comme
 * widget_edit.c. Jusqu'à la 2.8.1, la valeur exportée sortait échappée
 * (mesuré : « « » rendu « &laquo; »). */
static void texte_poser(Evas_Object *en, const char *t)
{
    char *m = elm_entry_utf8_to_markup(t ? t : "");
    elm_entry_entry_set(en, m ? m : "");
    free(m);
}
static gchar *texte_lire(Evas_Object *en)
{
    const char *m = elm_entry_entry_get(en);
    char *t = m ? elm_entry_markup_to_utf8(m) : NULL;
    gchar *r = g_strdup(t ? t : "");
    free(t);
    return r;
}

GtkWidget *widget_searchentry_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *en = elm_entry_add(win);
    elm_entry_single_line_set(en, EINA_TRUE);
    elm_object_part_text_set(en, "guide", "Rechercher...");
    Evas_Object *ic = elm_icon_add(en);
    elm_icon_standard_set(ic, "edit-find");
    elm_object_part_content_set(en, "icon", ic);
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) texte_poser(en, def);
    }
    evas_object_show(en);
    return (GtkWidget *)en;
}
gchar *widget_searchentry_envvar_construct(GtkWidget *w)
{ return texte_lire((Evas_Object *)w); }
gchar *widget_searchentry_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_searchentry_envvar_construct(v->Widget) : NULL; }
void widget_searchentry_clear(variable *v)
{ if (v && v->Widget) elm_entry_entry_set((Evas_Object *)v->Widget, ""); }
/* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo. Première
 * ligne = texte du champ (posé en texte brut). Au premier passage, un <default>
 * l'emporte : l'étalon l'applique après. Aucune action n'est branchée sur
 * « changed » dans ce port : la pose n'en joue pas. Jusqu'au 2026-10-13 il
 * n'était jamais lu. */
void widget_searchentry_refresh(variable *v)
{
    gchar *ligne;
    gboolean premier;
    GList *el = NULL;

    if (!v || !v->Widget) return;
    premier = widget_input_premier(v->Widget);
    ligne = widget_input_line(v->Attributes);
    if (!ligne) return;
    if (!(premier && attributeset_get_first(&el, v->Attributes, ATTR_DEFAULT)))
        texte_poser((Evas_Object *)v->Widget, ligne);
    g_free(ligne);
}
void widget_searchentry_fileselect(variable *v, const char *n, const char *val) {}
void widget_searchentry_removeselected(variable *v) {}
void widget_searchentry_save(variable *v) {}
