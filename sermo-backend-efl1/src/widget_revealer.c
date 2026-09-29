/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_revealer.c — Un enfant qui se montre et se cache (EFL/Elementary)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ Elementary montre et cache (evas_object_show/hide) mais n'anime pas
 * l'apparition : l'attribut transition= est accepté et IGNORÉ ici. L'état
 * exporté, lui, est identique à l'étalon.
 *
 * L'état vit dans les données de l'objet : création et export sont dans deux
 * fichiers différents (leçon du 2026-09-14). */
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
#include "widget_revealer.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

GtkWidget *widget_revealer_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;

    Evas_Object *parent = efl_main_win_get();
    Evas_Object *boite = elm_box_add(parent ? parent
                                            : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
    int montre = 0, poses = 0;

    if (attr) {
        const char *v = get_tag_attribute(attr, "reveal");
        if (v && (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || atoi(v) == 1))
            montre = 1;
    }
    if (Attr && attributeset_is_avail(Attr, ATTR_DEFAULT)) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (!strcasecmp(def, "true") || !strcasecmp(def, "yes") || atoi(def) == 1))
            montre = 1;
    }

    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n) {
        Evas_Object *c = (Evas_Object *) s.widgets[n];
        if (!c) continue;
        if (poses == 0) {
            evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
            evas_object_size_hint_align_set(c, EVAS_HINT_FILL, EVAS_HINT_FILL);
            elm_box_pack_end(boite, c);
            evas_object_show(c);
        } else {
            fprintf(stderr, "efl1sermo: <revealer> ne prend QU'UN enfant : le %de "
                            "est ignoré. Emballer le surplus dans une <vbox>.\n",
                    poses + 1);
        }
        poses++;
    }

    evas_object_data_set(boite, "sermo_montre", (void *)(intptr_t) montre);
    if (montre) evas_object_show(boite);
    else        evas_object_hide(boite);
    return (GtkWidget *) boite;
}

/* Export : l'état courant, « true » ou « false » (étalon gtk3). */
gchar *widget_revealer_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("false");
    return g_strdup((int)(intptr_t) evas_object_data_get((Evas_Object *) widget,
                                                         "sermo_montre")
                    ? "true" : "false");
}
gchar *widget_revealer_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_revealer_envvar_construct(var->Widget);
}
void widget_revealer_clear(variable *var)
{
    if (!var || !var->Widget) return;
    evas_object_data_set((Evas_Object *) var->Widget, "sermo_montre", (void *)(intptr_t) 0);
    evas_object_hide((Evas_Object *) var->Widget);
}
void widget_revealer_refresh(variable *var)        { (void) var; }
void widget_revealer_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_revealer_removeselected(variable *var) { (void) var; }
void widget_revealer_save(variable *var)           { (void) var; }
