/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_list.c — Liste EFL (elm_genlist simple)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "sermo_focus.h"
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_list.h"
#include "safe_exec.h"
#include "jouer_actions.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Item class simple pour elm_genlist */
static char *_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
    return strdup((const char *)data);
}

/* Chaque rangée porte une copie strdup() de son texte : la libérer quand la
 * rangée disparaît. Sans elle, chaque clear: ou refresh: perdait toutes les
 * copies — une liste rechargée en boucle grossissait sans fin. */
static void _gl_item_del(void *data, Evas_Object *obj)
{
    (void)obj;
    free(data);
}

static Elm_Genlist_Item_Class *_get_item_class(void)
{
    static Elm_Genlist_Item_Class *itc = NULL;
    if (!itc) {
        itc = elm_genlist_item_class_new();
        itc->item_style     = "default";
        itc->func.text_get  = _gl_text_get;
        itc->func.content_get = NULL;
        itc->func.state_get = NULL;
        itc->func.del       = _gl_item_del;
    }
    return itc;
}

static void _list_sel(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    const char *txt = elm_object_item_text_get(item);
    /* Une rangée pas encore dessinée (ajoutée par append: dans une liste
     * vide) n'a pas de texte d'objet : celui de sa donnée, qui est le même.
     * Sans cela strdup(NULL) arrêtait le programme. */
    if (!txt) txt = (const char *)elm_object_item_data_get(item);
    if (!txt) txt = "";
    /* Construire la sélection (accumulation séparée par |) */
    char *cur = (char *)evas_object_data_get(obj, "selected");
    if (cur && *cur) {
        size_t nl = strlen(cur)+strlen(txt)+2;
        char *ns  = malloc(nl);
        snprintf(ns, nl, "%s|%s", cur, txt);
        free(cur);
        evas_object_data_set(obj, "selected", ns);
    } else {
        free(cur);
        evas_object_data_set(obj, "selected", strdup(txt));
    }
    /* Actions : jouées à chaque sélection, comme le signal « selection-changed »
     * de l'étalon. */
    if (data)
        jouer_actions((GtkWidget *)obj, (AttributeSet *)data, "selection-changed", "row-selected");
}

/* La sélection du chargement (première rangée, ou la première ligne de
 * <input>) n'est qu'une donnée posée sur la liste : elle est jouée une fois ici,
 * différée à la boucle — le cœur appelle widget_list_refresh() juste après la
 * création, et les variables que l'action vise ne sont pas encore construites. */
static void _list_chargee(void *data)
{
    Evas_Object *gl = (Evas_Object *)data;
    const char *sel = (const char *)evas_object_data_get(gl, "selected");
    AttributeSet *Attr = (AttributeSet *)evas_object_data_get(gl, "sermo-attr");
    if (Attr && sel && *sel)
        jouer_actions((GtkWidget *)gl, Attr, "selection-changed", "row-selected");
}

GtkWidget *widget_list_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *gl = elm_genlist_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));
    elm_genlist_multi_select_set(gl, EINA_TRUE);
    evas_object_data_set(gl, "selected", strdup(""));
    evas_object_smart_callback_add(gl, "selected", _list_sel, Attr);
    if (Attr) {
        evas_object_data_set(gl, "sermo-attr", Attr);
        ecore_job_add(_list_chargee, gl);
    }

    Elm_Genlist_Item_Class *itc = _get_item_class();
    /* La liste prend-elle le focus à l'ouverture ? (sermo_focus.h) */
    evas_object_data_set(gl, "sermo-focus", (void *)(intptr_t)sermo_focus_initial("list"));

    if (Attr) {
        GList *el = NULL;
        gchar *item = attributeset_get_first(&el, Attr, ATTR_ITEM);
        gchar *premier = NULL;
        while (item) {
            if (*item) {
                if (!premier) premier = item;
                elm_genlist_item_append(gl, itc, strdup(item), NULL,
                                       ELM_GENLIST_ITEM_NONE, NULL, NULL);
            }
            item = attributeset_get_next(&el, Attr, ATTR_ITEM);
        }
        /* Parité gtk3 : la ligne de <default> si elle existe ; sinon la 1re,
         * seulement si la liste prend le focus à l'ouverture (jusqu'à la
         * 2.8.1 : toujours la 1re, et <default> ignoré). */
        GList *ed = NULL;
        gchar *def = attributeset_get_first(&ed, Attr, ATTR_DEFAULT);
        const gchar *choix = NULL;
        if (def && *def) {
            GList *e2 = NULL;
            for (gchar *it = attributeset_get_first(&e2, Attr, ATTR_ITEM); it && !choix;
                 it = attributeset_get_next(&e2, Attr, ATTR_ITEM))
                if (strcmp(it, def) == 0) choix = it;
        }
        if (!choix && evas_object_data_get(gl, "sermo-focus")) choix = premier;
        if (choix) {
            free(evas_object_data_get(gl, "selected"));
            evas_object_data_set(gl, "selected", strdup(choix));
        }
        /* <input> : lu par widget_list_refresh(), que le cœur appelle juste
         * après la création (lu ici aussi, la commande tournerait deux fois). */
    }
    evas_object_show(gl);
    return (GtkWidget *)gl;
}

gchar *widget_list_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("");
    const char *sel = (const char *)evas_object_data_get((Evas_Object *)w, "selected");
    return g_strdup(sel ? sel : "");
}
gchar *widget_list_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_list_envvar_construct(var->Widget);
}
void widget_list_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_genlist_clear((Evas_Object *)var->Widget);
    free(evas_object_data_get((Evas_Object *)var->Widget, "selected"));
    evas_object_data_set((Evas_Object *)var->Widget, "selected", strdup(""));
}
void widget_list_refresh(variable *var)
{
    Evas_Object *gl;
    gchar **lines, **p;
    int vide;

    if (!var || !var->Widget || !var->Attributes) return;
    gl = (Evas_Object *)var->Widget;
    /* <input> (commande ou fichier) : chaque ligne, vide comprise, AJOUTE une
     * rangée — l'étalon gtk3sermo ne vide pas une liste au refresh. */
    lines = widget_input_lines(var->Attributes);
    if (!lines) return;
    vide = elm_genlist_first_item_get(gl) == NULL;
    for (p = lines; *p; p++)
        elm_genlist_item_append(gl, _get_item_class(), strdup(*p), NULL,
                                ELM_GENLIST_ITEM_NONE, NULL, NULL);
    /* Comme après <item> : une liste jusque-là vide exporte sa 1re rangée. */
    if (vide && lines[0] && evas_object_data_get(gl, "sermo-focus")) {
        free(evas_object_data_get(gl, "selected"));
        evas_object_data_set(gl, "selected", strdup(lines[0]));
    }
    g_strfreev(lines);
}
void widget_list_fileselect(variable *var, const char *n, const char *v) {}
void widget_list_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    Evas_Object *gl = (Evas_Object *)var->Widget;
    Elm_Object_Item *sel = elm_genlist_selected_item_get(gl);
    if (sel) elm_object_item_del(sel);
}
void widget_list_save(variable *var) {}

/* ─── append: et rangées de save: (contrat, sermo-contract.h) ────────────────
 * Le cœur neutre ne sait pas toucher un widget EFL : jusqu'à la 3.3.0,
 * append: n'y faisait rien et save: n'écrivait pas les listes. Même règle que
 * l'étalon gtk3sermo : <entry>, le texte est remplacé ; <edit>, il est ajouté
 * à la fin ; <list>, une rangée est ajoutée EN TÊTE et choisie. */
void efl1_entry_poser(GtkWidget *w, const char *texte);
void efl1_edit_ajouter(GtkWidget *w, const char *texte);
char *efl1_tree_rangees(GtkWidget *w);
int   efl1_tree_ajouter(GtkWidget *w, const char *valeur);
char *efl1_table_rangees(GtkWidget *w);
int   efl1_table_ajouter(GtkWidget *w, const char *valeur);
int   sermo_be_widget_append(GtkWidget *w, int type, const char *valeur);
char *sermo_be_widget_rows(GtkWidget *w, int type);

int sermo_be_widget_append(GtkWidget *w, int type, const char *valeur)
{
    if (!w || !valeur) return 0;
    switch (type) {
    case WIDGET_ENTRY:
        efl1_entry_poser(w, valeur);
        return 1;
    case WIDGET_EDIT:
        efl1_edit_ajouter(w, valeur);
        return 1;
    case WIDGET_LIST: {
        Evas_Object *gl = (Evas_Object *)w;
        Eina_List *choisis = eina_list_clone(elm_genlist_selected_items_get(gl));
        Elm_Object_Item *it;
        EINA_LIST_FREE(choisis, it)
            elm_genlist_item_selected_set(it, EINA_FALSE);
        free(evas_object_data_get(gl, "selected"));
        evas_object_data_set(gl, "selected", strdup(""));
        it = elm_genlist_item_prepend(gl, _get_item_class(), strdup(valeur), NULL,
                                      ELM_GENLIST_ITEM_NONE, NULL, NULL);
        if (it) elm_genlist_item_selected_set(it, EINA_TRUE);
        /* la valeur exportée : la rangée ajoutée, que le rappel de sélection
         * ait joué ou non (moteur sans affichage) */
        free(evas_object_data_get(gl, "selected"));
        evas_object_data_set(gl, "selected", strdup(valeur));
        return 1;
    }
    case WIDGET_TREE:
        return efl1_tree_ajouter(w, valeur);     /* une rangée À LA FIN */
    case WIDGET_TABLE:
        return efl1_table_ajouter(w, valeur);    /* de même */
    default:
        return 0;
    }
}

char *sermo_be_widget_rows(GtkWidget *w, int type)
{
    size_t taille = 1, pos = 0;
    char *r = (char *)malloc(1);

    if (!w || !r) { free(r); return NULL; }
    r[0] = '\0';
    /* rangées brutes « a|b » : le cœur les ramène aux colonnes du <label> */
    if (type == WIDGET_TREE)  { free(r); return efl1_tree_rangees(w); }
    if (type == WIDGET_TABLE) { free(r); return efl1_table_rangees(w); }
    if (type == WIDGET_LIST) {
        Elm_Object_Item *it;
        for (it = elm_genlist_first_item_get((Evas_Object *)w); it;
             it = elm_genlist_item_next_get(it)) {
            const char *t = (const char *)elm_object_item_data_get(it);
            size_t l = t ? strlen(t) : 0;
            char *n = (char *)realloc(r, taille + l + 1);
            if (!n) { free(r); return NULL; }
            r = n;
            if (pos) r[pos++] = '\n';
            if (l) memcpy(r + pos, t, l);
            pos += l; r[pos] = '\0'; taille = pos + 1;
        }
    } else if (type == WIDGET_COMBOBOXTEXT || type == WIDGET_COMBOBOXENTRY) {
        const Eina_List *l;
        Elm_Object_Item *it;
        EINA_LIST_FOREACH(elm_hoversel_items_get((Evas_Object *)w), l, it) {
            const char *t = elm_object_item_text_get(it);
            size_t lg = t ? strlen(t) : 0;
            char *n = (char *)realloc(r, taille + lg + 1);
            if (!n) { free(r); return NULL; }
            r = n;
            if (pos) r[pos++] = '\n';
            if (lg) memcpy(r + pos, t, lg);
            pos += lg; r[pos] = '\0'; taille = pos + 1;
        }
    } else {
        free(r);
        return NULL;
    }
    return r;
}
