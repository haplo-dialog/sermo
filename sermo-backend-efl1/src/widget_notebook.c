/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_notebook.c — Onglets EFL (elm_box + elm_toolbar)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * EFL n'a pas de GtkNotebook direct. On utilise elm_toolbar (onglets) +
 * elm_box pour le contenu ; chaque page est un enfant de la pile.
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
#include "widget_notebook.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    Evas_Object *content;
    char        *label;
} NbPage;

static void _tab_selected(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *it = (Elm_Object_Item *)event_info;
    /* Elementary passe à la fonction d'un élément de barre la DONNÉE de
     * l'élément (ici son numéro, posé par elm_object_item_data_set), pas celle
     * donnée à elm_toolbar_item_append : le carnet se retrouve par la barre
     * (obj). Jusqu'au 2026-10-13, data valait le numéro et le carnet était
     * pris pour l'objet 0x1 (« Eo ID 0x1 is not a valid object »). */
    Evas_Object *nb = (Evas_Object *)evas_object_data_get(obj, "sermo_notebook");
    (void)data;
    if (!nb) return;
    /* Cacher toutes les pages, montrer la sélectionnée */
    int idx = (int)(intptr_t)elm_object_item_data_get(it);
    int n = (int)(intptr_t)evas_object_data_get(nb, "page_count");
    /* Déjà la page courante (et montrée) : rien à refaire. Évite aussi de
     * réempaqueter la page quand refresh choisit l'onglet puis l'appelle. */
    if (idx == (int)(intptr_t)evas_object_data_get(nb, "current_page")) return;
    /* Une box elm reserve la place de ses enfants CACHES : toutes les
     * pages s'empilaient. On ne garde empaquetee que la page courante. */
    Evas_Object *content = (Evas_Object *)evas_object_data_get(nb, "content");
    for (int i = 0; i < n; i++) {
        char key[32]; snprintf(key, sizeof(key), "page_%d", i);
        Evas_Object *pg = (Evas_Object *)evas_object_data_get(nb, key);
        if (!pg) continue;
        if (i == idx) {
            evas_object_size_hint_weight_set(pg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
            evas_object_size_hint_align_set(pg, EVAS_HINT_FILL, EVAS_HINT_FILL);
            evas_object_data_del(pg, "nb_hidden");
            if (content) elm_box_pack_end(content, pg);
            evas_object_show(pg);
        } else {
            evas_object_data_set(pg, "nb_hidden", (void *)1);
            if (content) elm_box_unpack(content, pg);
            evas_object_hide(pg);
        }
    }
    evas_object_data_set(nb, "current_page", (void *)(intptr_t)idx);
}

GtkWidget *widget_notebook_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *win_p  = parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC);

    /* tab-pos="left" : barre d'onglets VERTICALE a gauche du contenu */
    const char *tp = attr ? get_tag_attribute(attr, "tab-pos") : NULL;
    int side = tp && (!strcasecmp(tp, "left") || !strcasecmp(tp, "right") ||
                      !strcasecmp(tp, "start") || !strcmp(tp, "0"));

    /* Conteneur principal = box(toolbar + content_area), verticale ou non */
    Evas_Object *vbox = elm_box_add(win_p);
    elm_box_horizontal_set(vbox, side ? EINA_TRUE : EINA_FALSE);

    Evas_Object *tb = elm_toolbar_add(win_p);
    elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
    if (side) {
        elm_toolbar_horizontal_set(tb, EINA_FALSE);
        elm_toolbar_align_set(tb, 0.0);
        evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(tb, 0.0, EVAS_HINT_FILL);
    } else {
        evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);
    }
    elm_box_pack_end(vbox, tb);
    evas_object_show(tb);

    /* Étiquettes d'onglets : attribut tab-labels="A|B|C" (labels= déprécié),
     * comme le port GTK de référence. */
    gchar **labels = NULL;
    int     nlabels = 0;
    if (attr) {
        const char *v = get_tag_attribute(attr, "tab-labels");
        if (!v) v = get_tag_attribute(attr, "labels");   /* Déprécié */
        if (v) {
            labels = g_strsplit(v, "|", -1);
            while (labels[nlabels]) nlabels++;
        }
    }

    /* Pages : UN seul élément de pile, coalescé par l'instruction SUM de
     * l'automate (npages = nwidgets), comme le port GTK de référence.
     * widgets[i] est la i-ème page dans l'ordre du document. */
    stackelement s = pop();
    int npages = s.nwidgets;

    /* Zone de contenu */
    Evas_Object *content = elm_box_add(win_p);
    elm_box_horizontal_set(content, EINA_FALSE);
    evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(content, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_box_pack_end(vbox, content);
    evas_object_show(content);

    evas_object_data_set(vbox, "toolbar", tb);
    evas_object_data_set(tb, "sermo_notebook", vbox);   /* lu par _tab_selected */
    evas_object_data_set(vbox, "content", content);
    evas_object_data_set(vbox, "page_count", (void *)(intptr_t)npages);
    evas_object_data_set(vbox, "current_page", (void *)(intptr_t)0);

    for (int i = 0; i < npages; ++i) {
        char key[32]; snprintf(key, sizeof(key), "page_%d", i);
        Evas_Object *pg = (Evas_Object *)s.widgets[i];
        evas_object_data_set(vbox, key, pg);
        evas_object_size_hint_weight_set(pg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(pg, EVAS_HINT_FILL, EVAS_HINT_FILL);
        if (i == 0) { elm_box_pack_end(content, pg); evas_object_show(pg); }
        else        { evas_object_data_set(pg, "nb_hidden", (void *)1);
                      evas_object_hide(pg); }   /* empaquetee a la selection */

        char fallback[32];
        const char *lbl;
        if (i < nlabels) {
            lbl = labels[i];
        } else {
            snprintf(fallback, sizeof(fallback), "Page %d", i + 1);
            lbl = fallback;
        }
        Elm_Object_Item *ti = elm_toolbar_item_append(tb, NULL,
            lbl, _tab_selected, vbox);
        elm_object_item_data_set(ti, (void *)(intptr_t)i);
    }

    if (labels) g_strfreev(labels);

    evas_object_show(vbox);
    return (GtkWidget *)vbox;
}

gchar *widget_notebook_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("0");
    int cur = (int)(intptr_t)evas_object_data_get((Evas_Object *)w, "current_page");
    char buf[32]; snprintf(buf, sizeof(buf), "%d", cur);
    return g_strdup(buf);
}
gchar *widget_notebook_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_notebook_envvar_construct(var->Widget);
}
void widget_notebook_clear(variable *var) {}
/* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo. Première
 * ligne = numéro de page (atoi) → onglet choisi, page montrée, valeur
 * exportée. Comme gtk_notebook_set_current_page() : négatif = dernière page,
 * au-delà du nombre de pages = rien. Premier passage : voir le refresh. Aucune action n'est
 * branchée sur les onglets : la pose n'en joue pas. Jusqu'au 2026-10-13 il
 * n'était jamais lu. */
void widget_notebook_refresh(variable *var)
{
    Evas_Object *nb, *tb;
    Elm_Object_Item *it;
    gchar *ligne;
    gboolean premier;
    int idx, n, i;

    if (!var || !var->Widget) return;
    nb = (Evas_Object *)var->Widget;
    premier = widget_input_premier(var->Widget);
    ligne = widget_input_line(var->Attributes);
    /* Premier passage : l'<input> pose la page, sauf si la balise porte
     * page="…", qui l'emporte — règle de gtk4sermo, et de gtk3sermo depuis le
     * 2026-10-18 (il perdait cette page, GtkNotebook la refusant avant
     * l'affichage). Jusque-là ce port copiait cette perte. */
    if (premier && (var->widget_tag_attr ? get_tag_attribute(var->widget_tag_attr, "page") : NULL)) {
        g_free(ligne);
        ligne = g_strdup((var->widget_tag_attr ? get_tag_attribute(var->widget_tag_attr, "page") : NULL));
    }
    if (!ligne) return;
    idx = atoi(ligne);
    g_free(ligne);
    n = (int)(intptr_t)evas_object_data_get(nb, "page_count");
    if (n <= 0) return;
    if (idx < 0) idx = n - 1;
    if (idx >= n) return;
    tb = (Evas_Object *)evas_object_data_get(nb, "toolbar");
    if (!tb) return;
    for (it = elm_toolbar_first_item_get(tb), i = 0; it && i < idx;
         it = elm_toolbar_item_next_get(it), i++)
        ;
    if (!it) return;
    elm_toolbar_item_selected_set(it, EINA_TRUE);
    /* Onglet déjà choisi dans la barre : son rappel n'a pas joué. */
    _tab_selected(nb, tb, it);
}
void widget_notebook_fileselect(variable *var, const char *n, const char *v) {}
void widget_notebook_removeselected(variable *var) {}
void widget_notebook_save(variable *var) {}
