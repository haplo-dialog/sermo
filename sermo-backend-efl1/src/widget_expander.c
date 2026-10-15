/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_expander.c — Expander EFL (elm_frame collapsible)
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
#include "widget_expander.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>


/* Même règle que l'étalon (gtk3 widget_expander.c) : l'état initial vient de
 * l'ATTRIBUT DE BALISE expanded= — « true », « yes » ou 1 — et un expander sans
 * cet attribut est REPLIÉ. Ce port lisait <default> (que l'étalon ignore) et
 * partait ouvert : le cas 40 du banc mesure les deux écarts. */
static gboolean expander_ouvert_au_depart(tag_attr *attr)
{
    const char *v = attr ? get_tag_attribute(attr, "expanded") : NULL;
    if (!v) return FALSE;
    return (strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 || atoi(v) == 1);
}

GtkWidget *widget_expander_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *fr = elm_frame_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    elm_frame_collapse_go(fr, expander_ouvert_au_depart(attr) ? EINA_FALSE : EINA_TRUE);
    /* Lu par refresh : l'attribut expanded= l'emporte sur l'<input> au
     * premier passage (l'étalon le pose après). */
    if (attr && get_tag_attribute(attr, "expanded"))
        evas_object_data_set(fr, "sermo_expanded_attr", (void *)1);

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) elm_object_text_set(fr, lbl);
    }
    /* Le titre s'écrit aussi en attribut, <expander label="…"> : l'étalon gtk3 le
     * lit (propriété GtkExpander:label), ce port l'ignorait (mesuré sur les
     * captures du site, 2026-10-10). */
    if (attr) { const char *v = get_tag_attribute(attr, "label"); if (v && *v) elm_object_text_set(fr, v); }

    stackelement s = pop();
    if (s.widgets[0]) {
        elm_object_content_set(fr, (Evas_Object *)s.widgets[0]);
        evas_object_show((Evas_Object *)s.widgets[0]);
    }
    evas_object_show(fr);
    return (GtkWidget *)fr;
}

gchar *widget_expander_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("false");
    return g_strdup(elm_frame_collapse_get((Evas_Object *)w) ? "false" : "true");
}
gchar *widget_expander_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_expander_envvar_construct(var->Widget);
}
void widget_expander_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_frame_collapse_go((Evas_Object *)var->Widget, EINA_TRUE);
}
/* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo. Première
 * ligne true/yes/1 (sans casse, ou atoi()==1) → déplié, toute autre → replié.
 * Au premier passage, l'attribut expanded= l'emporte (la commande tourne
 * quand même) : l'étalon le pose après. Aucune action n'est branchée sur le
 * cadre dans ce port : la pose n'en joue pas. Jusqu'au 2026-10-13 il n'était
 * jamais lu. */
void widget_expander_refresh(variable *var)
{
    gint v;
    gboolean premier;

    if (!var || !var->Widget) return;
    premier = widget_input_premier(var->Widget);
    v = widget_input_bool(var->Attributes);
    if (v < 0) return;
    if (premier && evas_object_data_get((Evas_Object *)var->Widget, "sermo_expanded_attr"))
        return;
    elm_frame_collapse_go((Evas_Object *)var->Widget, v ? EINA_FALSE : EINA_TRUE);
}
void widget_expander_fileselect(variable *var, const char *n, const char *v) {}
void widget_expander_removeselected(variable *var) {}
void widget_expander_save(variable *var) {}
