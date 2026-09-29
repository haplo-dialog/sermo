/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_combobox.c — Liste déroulante EFL (elm_combobox / elm_hoversel)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * EFL < 1.17 : utilise elm_hoversel comme fallback.
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
#include "widget_combobox.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void _item_selected(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    const char *label = elm_object_item_text_get(item);
    evas_object_data_set(obj, "selected", (void *)label);
}

GtkWidget *widget_combobox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *hs = elm_hoversel_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    evas_object_smart_callback_add(hs, "selected", _item_selected, NULL);

    const char *def_val = NULL;
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) def_val = def;
        el = NULL;
        gchar *item = attributeset_get_first(&el, Attr, ATTR_ITEM);
        while (item) {
            if (*item) elm_hoversel_item_add(hs, item, NULL, ELM_ICON_NONE, NULL, NULL);
            item = attributeset_get_next(&el, Attr, ATTR_ITEM);
        }
        /* <input> : voir widget_combobox_refresh() */
    }
    if (def_val) {
        elm_object_text_set(hs, def_val);
        evas_object_data_set(hs, "selected", (void *)def_val);
    }
    evas_object_show(hs);
    return (GtkWidget *)hs;
}

gchar *widget_combobox_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("");
    const char *sel = (const char *)evas_object_data_get((Evas_Object *)w, "selected");
    return g_strdup(sel ? sel : "");
}
gchar *widget_combobox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_combobox_envvar_construct(var->Widget);
}
void widget_combobox_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_hoversel_clear((Evas_Object *)var->Widget);
}
void widget_combobox_refresh(variable *var)
{
    GList *el = NULL;
    gchar *act;

    if (!var || !var->Attributes) return;
    /* L'étalon gtk3sermo n'implémente PAS <input> pour <combobox> : ni
     * commande lancée ni fichier lu, un avertissement à la place. L'ancienne
     * lecture, elle, passait « Command:… » tel quel au shell. */
    for (act = attributeset_get_first(&el, var->Attributes, ATTR_INPUT); act;
         act = attributeset_get_next(&el, var->Attributes, ATTR_INPUT)) {
        if (input_is_shell_command(act))
            g_warning("%s(): <input> not implemented for this widget.", __func__);
        else if (strncasecmp(act, "file:", 5) == 0 && act[5] != '\0')
            g_warning("%s(): <input file> not implemented for this widget.", __func__);
    }
}
void widget_combobox_fileselect(variable *var, const char *n, const char *v) {}
void widget_combobox_removeselected(variable *var) {}
void widget_combobox_save(variable *var) {}
