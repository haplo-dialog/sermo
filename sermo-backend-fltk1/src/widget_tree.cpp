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
#include "sermo_focus.h"
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "safe_exec.h"
#include "widget_tree.h"
#include <FL/Fl_Tree.H>
#include <FL/fl_draw.H>
#include <vector>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Colonnes « a|b|c » (format de l'étalon) : Fl_Tree n'en a pas. L'arbre
 * retient la largeur de chaque colonne, et un élément sur mesure dessine ses
 * cellules. Jusqu'à la 2.8.0, « | » restait dans le texte, et « / » d'un
 * chemin (« /floppy/ ») ouvrait des niveaux d'arborescence. */
class SermoTree : public Fl_Tree {
public:
    std::vector<int> larg;
    SermoTree(int x, int y, int w, int h) : Fl_Tree(x, y, w, h) {}
};

class ColItem : public Fl_Tree_Item {
public:
    ColItem(Fl_Tree *t) : Fl_Tree_Item(t) {}
    int draw_item_content(int render) FL_OVERRIDE {
        SermoTree *t = (SermoTree *)tree();
        int X = label_x(), Y = label_y(), W = label_w(), H = label_h();
        if (render) {
            if (is_selected()) fl_draw_box(prefs().selectbox(), X, Y, W, H, drawbgcolor());
            else { fl_color(drawbgcolor()); fl_rectf(X, Y, W, H); }
        }
        fl_font(labelfont(), labelsize());
        const char *l = label() ? label() : "";
        int x = X, col = 0;
        while (1) {
            size_t n = strcspn(l, "|");
            int w = col < (int)t->larg.size() ? t->larg[col] : (int)fl_width(l, (int)n) + 12;
            if (render) {
                fl_color(drawfgcolor());
                fl_push_clip(x, Y, w - 4, H);
                std::string cell(l, n);
                fl_draw(cell.c_str(), x + 2, Y, w - 6, H, FL_ALIGN_LEFT);
                fl_pop_clip();
            }
            x += w;
            if (!l[n]) break;
            l += n + 1;
            col++;
        }
        return x;
    }
};

static void tree_ajouter(Fl_Tree *tree, const char *texte, int entete)
{
    /* add(parent, libellé) : le texte tel quel (« / » n'y ouvre pas de
     * niveau) ; puis l'élément à colonnes prend sa place (replace(), comme
     * l'exemple tree-custom-draw-items de FLTK) — ajouté directement à la
     * racine, l'arbre ne le dessinait pas. */
    Fl_Tree_Item *it = tree->add(tree->root(), texte);
    if (!it) return;
    ColItem *ci = new ColItem(tree);
    ci->label(texte);
    it = it->replace(ci);
    if (it && entete) it->deactivate();   /* les en-têtes ne se choisissent pas */
}

/* Largeur de chaque colonne : la plus longue cellule, en-tête compris. */
static void tree_mesurer(Fl_Tree *tree)
{
    SermoTree *t = (SermoTree *)tree;
    t->larg.clear();
    fl_font(tree->item_labelfont(), tree->item_labelsize());
    for (Fl_Tree_Item *it = tree->first(); it; it = tree->next(it)) {
        if (it == tree->root() || !it->label() || !strchr(it->label(), '|')) continue;
        const char *l = it->label();
        for (int col = 0;; col++) {
            size_t n = strcspn(l, "|");
            int w = (int)fl_width(l, (int)n) + 16;
            if (col >= (int)t->larg.size()) t->larg.push_back(w);
            else if (w > t->larg[col]) t->larg[col] = w;
            if (!l[n]) break;
            l += n + 1;
        }
    }
}

/* <label>Col1|Col2</label> : une rangée d'en-têtes en tête de l'arbre. */
static void tree_entete(Fl_Tree *tree, AttributeSet *Attr)
{
    GList *el = NULL;
    gchar *lbl = Attr ? attributeset_get_first(&el, Attr, ATTR_LABEL) : NULL;
    if (lbl && strchr(lbl, '|')) tree_ajouter(tree, lbl, 1);
}

/* <item> : « / » y marque la hiérarchie (extension du port) — sauf dans une
 * ligne à colonnes « a|b|c », où « / » est du texte (un chemin). */
static void tree_add_items(Fl_Tree *tree, AttributeSet *Attr)
{
    GList *el = NULL;
    gchar *item = attributeset_get_first(&el, Attr, ATTR_ITEM);
    while (item) {
        if (*item) {
            if (strchr(item, '|')) tree_ajouter(tree, item, 0);
            else                   tree->add(item);
        }
        item = attributeset_get_next(&el, Attr, ATTR_ITEM);
    }
}

/* Parité étalon : la première ligne est sélectionnée par défaut
 * (TR="a" sur le banc, comme gtk3sermo) — après <item> comme après <input>. */
/* Arbres qui prennent le focus à l'ouverture (sermo_focus.h) : seuls ceux-là
 * ont leur 1re ligne choisie, comme gtk3sermo (jusqu'à la 2.8.1 : tous). */
#include <map>
#include <set>
static std::set<const Fl_Tree *> arbres_focus;
/* <tree exported-column="N"> : la colonne que la variable rend (défaut 0). */
static std::map<const Fl_Widget *, int> arbres_colonne;

static void tree_select_first(Fl_Tree *tree)
{
    if (tree->first_selected_item() || !arbres_focus.count(tree)) return;
    Fl_Tree_Item *it = tree->first();
    while (it && (it == tree->root() || !it->is_active())) it = tree->next(it);   /* pas l'en-tête */
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
    Fl_Tree *tree = new SermoTree(0, 0, ww, hh);
    tree->showroot(0);
    tree->selectmode(FL_TREE_SELECT_SINGLE);

    /* <input> : dans refresh, que le cœur appelle juste après la création
     * (lu ici aussi, la commande tournait deux fois). */
    if (Attr) { tree_entete(tree, Attr); tree_add_items(tree, Attr); }
    tree_mesurer(tree);
    if (sermo_focus_initial("tree")) arbres_focus.insert(tree);
    if (attr) {
        const char *v = get_tag_attribute(attr, "exported-column");
        if (v && atoi(v) > 0) arbres_colonne[tree] = atoi(v);
    }
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
    if (sel->parent() == tree->root()) {   /* exported-column, comme l'étalon */
        const char *l = sel->label() ? sel->label() : "";
        auto c = arbres_colonne.find(tree);
        for (int n = c == arbres_colonne.end() ? 0 : c->second; n > 0; n--) {
            l = strchr(l, '|');
            if (!l) return g_strdup("");
            l++;
        }
        return g_strndup(l, strcspn(l, "|"));
    }
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
        tree_entete(tree, v->Attributes);
        /* une ligne entière, colonnes comprises ; « / » y reste du texte */
        for (gchar **l = lignes; *l; l++) tree_ajouter(tree, *l, 0);
        tree_add_items(tree, v->Attributes);
        tree_mesurer(tree);
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
