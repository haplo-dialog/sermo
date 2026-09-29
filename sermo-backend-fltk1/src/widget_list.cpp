/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_list.cpp — Liste de sélection FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <list> → Fl_Select_Browser (sélection simple)
 *          ou Fl_Multi_Browser si multiple-selection="true"
 *
 * Alimentation :
 *   - <item>texte</item>  → ATTR_ITEM (à la création)
 *   - <input> (commande ou fichier) → une ligne = une rangée AJOUTÉE (refresh)
 *
 * Export :
 *   - sélection simple : texte de la ligne sélectionnée
 *   - sélection multiple : lignes séparées par "|"
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_list.h"
#include "safe_exec.h"

#include <FL/Fl_Multi_Browser.H>
#include <FL/Fl_Select_Browser.H>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Sélection après chargement — <item> à la création, <input> au refresh :
 * la rangée égale à <default>, sinon (sélection simple) la PREMIÈRE — parité
 * étalon, LI="x" sur le banc comme gtk3sermo. Une sélection simple déjà faite
 * n'est pas déplacée par un refresh qui ajoute des rangées. */
static void list_select_default(Fl_Browser *br, AttributeSet *Attr)
{
    bool multi = (br->type() == FL_MULTI_BROWSER);

    if (!multi && br->value() > 0) return;

    GList *element = NULL;
    gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
    if (def && *def) {
        for (int i = 1; i <= br->size(); i++) {
            if (br->text(i) && strcmp(br->text(i), def) == 0) {
                br->select(i);
                break;
            }
        }
    }

    if (!multi && br->size() > 0 && br->value() <= 0)
        br->select(1);
}

GtkWidget *widget_list_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 200, h = 150;
    bool   multi = false;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
        if ((v = get_tag_attribute(attr, "multiple-selection")))
            multi = (strcasecmp(v, "true") == 0 || strcmp(v, "1") == 0);
    }

    Fl_Browser *br;
    if (multi) {
        br = new Fl_Multi_Browser(0, 0, w, h, nullptr);
    } else {
        br = new Fl_Select_Browser(0, 0, w, h, nullptr);
    }
    br->box(FL_DOWN_BOX);

    if (Attr) {
        /* Items statiques (<input> : dans refresh, que le cœur appelle juste
         * après la création — lu ici aussi, la commande tournait deux fois) */
        element = NULL;
        gchar *item = attributeset_get_first(&element, Attr, ATTR_ITEM);
        while (item) {
            if (*item) br->add(item);
            item = attributeset_get_next(&element, Attr, ATTR_ITEM);
        }
    }

    list_select_default(br, Attr);

    return (GtkWidget *)br;
}

gchar *widget_list_envvar_construct(GtkWidget *widget)
{
    Fl_Browser *br = (Fl_Browser *)widget;
    if (!br) return g_strdup("");

    /* Vérifier si c'est un multi-browser */
    bool multi = (br->type() == FL_MULTI_BROWSER);

    if (!multi) {
        /* Sélection simple */
        Fl_Select_Browser *sb = (Fl_Select_Browser *)br;
        int sel = sb->value();
        if (sel <= 0 || !sb->text(sel)) return g_strdup("");
        return g_strdup(sb->text(sel));
    } else {
        /* Sélection multiple : concaténer avec "|" */
        char *result = g_strdup("");
        for (int i = 1; i <= br->size(); i++) {
            if (br->selected(i)) {
                const char *txt = br->text(i);
                if (!txt) continue;
                if (*result) {
                    char *old = result;
                    result = g_strdup_printf("%s|%s", old, txt);
                    g_free(old);
                } else {
                    g_free(result);
                    result = g_strdup(txt);
                }
            }
        }
        return result;
    }
}

gchar *widget_list_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_list_envvar_construct(var->Widget);
}

void widget_list_clear(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Browser *br = (Fl_Browser *)var->Widget;
    br->deselect();
}

void widget_list_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Browser *br = (Fl_Browser *)var->Widget;

    /* <input> (commande ou fichier, décodé) : chaque ligne devient une rangée
     * AJOUTÉE — l'étalon ne vide pas la liste au refresh. */
    gchar **lignes = widget_input_lines(var->Attributes);
    if (lignes) {
        for (gchar **l = lignes; *l; l++)
            br->add(*l);
        g_strfreev(lignes);
        list_select_default(br, var->Attributes);
    }
    br->redraw();
}

void widget_list_fileselect(variable *var, const char *n, const char *v) {}

void widget_list_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Browser *br = (Fl_Browser *)var->Widget;
    /* Supprimer les éléments sélectionnés (du bas vers le haut) */
    for (int i = br->size(); i >= 1; i--) {
        if (br->selected(i)) br->remove(i);
    }
}

void widget_list_save(variable *var) {}
