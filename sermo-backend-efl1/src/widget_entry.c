/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_entry.c — Champ de saisie monoligne EFL
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
#include "widget_entry.h"
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_entry_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *en = elm_entry_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    elm_entry_single_line_set(en, EINA_TRUE);
    elm_entry_scrollable_set(en, EINA_TRUE);
    /* une entry scrollable a un min ~0 : lui donner un gabarit visible */
    evas_object_size_hint_min_set(en, 120, 27);

    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) elm_entry_entry_set(en, def);
    }
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "visibility")))
            if (strcasecmp(v,"false")==0 || strcmp(v,"0")==0)
                elm_entry_password_set(en, EINA_TRUE);
        if ((v = get_tag_attribute(attr, "max-length")))
            elm_entry_input_panel_layout_set(en, ELM_INPUT_PANEL_LAYOUT_NORMAL);
    }
    evas_object_show(en);
    return (GtkWidget *)en;
}

gchar *widget_entry_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("");
    const char *txt = elm_entry_entry_get((Evas_Object *)widget);
    return g_strdup(txt ? txt : "");
}
gchar *widget_entry_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_entry_envvar_construct(var->Widget);
}
void widget_entry_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_entry_entry_set((Evas_Object *)var->Widget, "");
}
void widget_entry_refresh(variable *var)
{
    gchar *text;
    if (!var || !var->Widget) return;
    /* Alimenter depuis <input> (Command:/file: decodes par le helper) */
    text = widget_input_text(var->Attributes);
    if (text) {
        /* la PREMIÈRE ligne, sans CR/LF — étalon gtk3sermo (elm_entry recollait
         * les lignes bout à bout : « un » + « deux » rendait « undeux ») */
        text[strcspn(text, "\r\n")] = '\0';
        elm_entry_entry_set((Evas_Object *)var->Widget, text);
        g_free(text);
    }
}
void widget_entry_fileselect(variable *var, const char *n, const char *v) {}
void widget_entry_removeselected(variable *var) {}
void widget_entry_save(variable *var) {}
