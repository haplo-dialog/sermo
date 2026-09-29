/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_tree.cpp — Arborescence FLTK (Fl_Tree)
 * sermo — haplo-dialog — GPL-2.0-or-later
 * Format <item> : "Parent/Enfant/Feuille"
 * <input> (commande ou fichier) : une ligne = une rangée de premier niveau,
 *   colonnes séparées par « | » — Fl_Tree n'en montre qu'une, la 1re
 * Export : chemin du nœud sélectionné (libellé seul au premier niveau) */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "safe_exec.h"
#include "widget_tree.h"
#include <FL/Fl_Tree.H>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* <item> : « / » y marque la hiérarchie (extension du port) */
static void tree_add_items(Fl_Tree *tree, AttributeSet *Attr)
{
    GList *el = NULL;
    gchar *item = attributeset_get_first(&el, Attr, ATTR_ITEM);
    while (item) {
        if (*item) tree->add(item);
        item = attributeset_get_next(&el, Attr, ATTR_ITEM);
    }
}

/* Parité étalon : la première ligne est sélectionnée par défaut
 * (TR="a" sur le banc, comme gtk3sermo) — après <item> comme après <input>. */
static void tree_select_first(Fl_Tree *tree)
{
    if (tree->first_selected_item()) return;
    Fl_Tree_Item *it = tree->first();
    if (it == tree->root()) it = tree->next(it);
    if (it) tree->select(it, 0);
}

/* Vider en GARDANT la racine : Fl_Tree::clear() la détruit aussi (mesuré,
 * FLTK 1.4.4), et add(root(), …) déréférencerait alors NULL. */
static void tree_empty(Fl_Tree *tree)
{
    if (tree->root()) tree->clear_children(tree->root());
}

GtkWidget *widget_tree_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int ww = 200, hh = 200;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  ww = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) hh = atoi(v);
    }
    Fl_Tree *tree = new Fl_Tree(0, 0, ww, hh);
    tree->showroot(0);
    tree->selectmode(FL_TREE_SELECT_SINGLE);

    /* <input> : dans refresh, que le cœur appelle juste après la création
     * (lu ici aussi, la commande tournait deux fois). */
    if (Attr) tree_add_items(tree, Attr);
    tree_select_first(tree);

    tree->redraw();
    return (GtkWidget *)tree;
}
gchar *widget_tree_envvar_construct(GtkWidget *w)
{
    Fl_Tree *tree = (Fl_Tree *)w;
    Fl_Tree_Item *sel = tree->first_selected_item();
    if (!sel) return g_strdup("");
    /* Premier niveau : le libellé tel quel. item_pathname() y échappe « / »
     * et « \ », et rend "" dès 1022 octets (mesuré, FLTK 1.4.4) : une ligne
     * de <input> ne ressortirait pas telle quelle. */
    if (sel->parent() == tree->root())
        return g_strdup(sel->label() ? sel->label() : "");
    char path[1024] = "";
    tree->item_pathname(path, sizeof(path), sel);
    return g_strdup(path);
}
gchar *widget_tree_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_tree_envvar_construct(v->Widget) : NULL; }
void widget_tree_clear(variable *v)
{ if (v && v->Widget) { tree_empty((Fl_Tree *)v->Widget); ((Fl_Tree *)v->Widget)->redraw(); } }
void widget_tree_refresh(variable *v)
{
    if (!v || !v->Widget) return;
    Fl_Tree *tree = (Fl_Tree *)v->Widget;

    gchar **lignes = widget_input_lines(v->Attributes);
    if (lignes && tree->root()) {
        /* Étalon : refresh VIDE l'arbre, puis <input>, puis <item>. */
        tree_empty(tree);
        for (gchar **l = lignes; *l; l++) {
            (*l)[strcspn(*l, "|")] = '\0';
            /* add(parent, libellé) prend le texte tel quel ; add(chemin)
             * aurait lu « / » comme une hiérarchie. */
            tree->add(tree->root(), *l);
        }
        tree_add_items(tree, v->Attributes);
        tree_select_first(tree);
    }
    g_strfreev(lignes);
    tree->redraw();
}
void widget_tree_fileselect(variable *v, const char*, const char*) {}
void widget_tree_removeselected(variable *v) {
    if (!v || !v->Widget) return;
    Fl_Tree *tree = (Fl_Tree *)v->Widget;
    Fl_Tree_Item *sel = tree->first_selected_item();
    if (sel) { tree->remove(sel); tree->redraw(); }
}
void widget_tree_save(variable *v) {}
