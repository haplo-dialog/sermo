/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_notebook.cpp — Onglets FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <notebook> → Fl_Tabs contenant des Fl_Group par onglet
 *
 * Modèle XML gtkdialog :
 *   <notebook>
 *     <vbox>...</vbox>  ← onglet 1  (label = ATTR_LABEL de la vbox ou "Tab N")
 *     <vbox>...</vbox>  ← onglet 2
 *   </notebook>
 *
 * Stratégie d'implémentation FLTK :
 *   - On crée un Fl_Tabs vide.
 *   - Chaque enfant poussé dans la pile est dépilé et encapsulé dans un
 *     Fl_Group portant le label de l'onglet.  Le titre est lu sur le widget
 *     enfant lui-même (child->label()) sinon "Tab N".
 *   - Les enfants sont dépilés jusqu'à ce que la pile soit vide pour ce niveau
 *     (le parseur garantit que push_widget / pop_widget sont appairés).
 *
 * Note : la profondeur de pile est gérée par automaton.c ; ici on dépile
 * tous les widgets disponibles au niveau courant (max 32 onglets).
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
#include "widget_notebook.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Wizard.H>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_TABS 32
#define TAB_BAR_H 25

/* Fl_Tabs place la barre en BAS des que l'espace libre sous les pages
 * depasse celui du dessus (Fl_Group::resize etire les pages en proportion).
 * On repositionne chaque page nous-memes : barre toujours en haut. */
class SermoTabs : public Fl_Tabs {
public:
    SermoTabs(int X, int Y, int W, int H) : Fl_Tabs(X, Y, W, H, nullptr) {}
    void resize(int X, int Y, int W, int H) override {
        Fl_Widget::resize(X, Y, W, H);
        for (int i = 0; i < children(); i++) {
            Fl_Group *pg = child(i)->as_group();
            if (!pg) continue;
            pg->resize(X, Y + TAB_BAR_H, W, H - TAB_BAR_H);
            if (pg->children() > 0)
                pg->child(0)->resize(X + 2, Y + TAB_BAR_H + 2, W - 4, H - TAB_BAR_H - 4);
        }
    }
};

/* Onglets a GAUCHE (tab-pos="left") : Fl_Tabs ne sait pas ; une colonne de
 * boutons a bascule + un Fl_Wizard (pile de pages). Le bouton actif est
 * enfonce, envvar = index de la page visible. */
class SermoSideTabs : public Fl_Group {
public:
    Fl_Flex   *col;
    Fl_Wizard *wiz;
    int        colw;
    SermoSideTabs(int X, int Y, int W, int H, int cw) : Fl_Group(X, Y, W, H, nullptr), colw(cw) {
        col = new Fl_Flex(X, Y, cw, H, Fl_Flex::COLUMN);
        col->gap(2); col->margin(2, 2); col->end();
        wiz = new Fl_Wizard(X + cw + 4, Y, W - cw - 4, H);
        wiz->box(FL_ENGRAVED_FRAME);
        wiz->end();
        end();
    }
    void resize(int X, int Y, int W, int H) override {
        Fl_Widget::resize(X, Y, W, H);
        col->resize(X, Y, colw, H);
        wiz->resize(X + colw + 4, Y, W - colw - 4, H);
        for (int i = 0; i < wiz->children(); i++)
            wiz->child(i)->resize(X + colw + 8, Y + 4, W - colw - 12, H - 8);
    }
    int current() const {
        Fl_Widget *v = wiz->value();
        for (int i = 0; i < wiz->children(); i++) if (wiz->child(i) == v) return i;
        return 0;
    }
    void select(int i) {
        if (i < 0 || i >= wiz->children()) return;
        wiz->value(wiz->child(i));
        for (int k = 0; k < col->children(); k++)
            ((Fl_Button *)col->child(k))->value(k == i);
        redraw();
    }
    static void tab_cb(Fl_Widget *b, void *) {
        SermoSideTabs *st = (SermoSideTabs *)b->parent()->parent();
        for (int k = 0; k < st->col->children(); k++)
            if (st->col->child(k) == b) { st->select(k); return; }
    }
};
static int SERMO_SIDETABS_TAG = 1;

/* hauteur naturelle d'une page (flex du port : somme/max des enfants) */
static int page_natural_h(Fl_Widget *c)
{
    Fl_Group *cg = c->as_group();
    if (cg && c->user_data() == &SERMO_FLEX_TAG && cg->children() > 0) {
        Fl_Flex *fx = (Fl_Flex *)c;
        int besoin = 0;
        for (int i = 0; i < cg->children(); i++) {
            int ch = cg->child(i)->h();
            if (!fx->horizontal()) besoin += ch + fx->gap();
            else if (ch > besoin) besoin = ch;
        }
        return besoin + 4;
    }
    return c->h();
}

GtkWidget *widget_notebook_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 400, h = 300;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    /* UN SEUL pop : le cœur fusionne les enfants en un élément (SUM),
     * déjà en ordre document. L'ancienne boucle « pop jusqu'à une
     * sentinelle » sous-vidait la pile : abort sur TOUT notebook
     * (System-Tools compris — 7 onglets jamais affichés). */
    Fl_Widget *children[MAX_TABS];
    int        nchildren = 0;
    {
        stackelement s = pop();
        for (int i = 0; i < s.nwidgets && nchildren < MAX_TABS; i++)
            if (s.widgets[i]) children[nchildren++] = (Fl_Widget *)s.widgets[i];
    }

    /* Libellés d'onglets : attribut tab-labels="A|B|C" (parité étalon) */
    gchar **tlabels = NULL;
    int     ntl = 0;
    if (attr) {
        const char *tl = get_tag_attribute(attr, "tab-labels");
        if (tl && *tl) {
            tlabels = g_strsplit(tl, "|", -1);
            while (tlabels[ntl]) ntl++;
        }
    }

    /* sans height-request : la hauteur de la page la plus haute */
    if (!(attr && get_tag_attribute(attr, "height-request"))) {
        int hmax = 0;
        for (int i = 0; i < nchildren; i++) {
            int ph = page_natural_h(children[i]);
            if (ph > hmax) hmax = ph;
        }
        if (hmax > 0) h = hmax + TAB_BAR_H + 4;
    }

    /* tab-pos="left" (ou "right"/"start") : onglets en colonne */
    const char *tp = attr ? get_tag_attribute(attr, "tab-pos") : NULL;
    if (tp && (strcasecmp(tp, "left") == 0 || strcasecmp(tp, "right") == 0 ||
               strcasecmp(tp, "start") == 0 || strcmp(tp, "0") == 0)) {
        int colw = 60;
        fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
        for (int i = 0; i < nchildren; i++) {
            const char *lbl = (i < ntl) ? tlabels[i] : children[i]->label();
            int tw = 0, th = 0;
            if (lbl && *lbl) fl_measure(lbl, tw, th, 0);
            if (tw + 24 > colw) colw = tw + 24;
        }
        SermoSideTabs *st = new SermoSideTabs(0, 0, w, h, colw);
        st->user_data(&SERMO_SIDETABS_TAG);
        for (int i = 0; i < nchildren; i++) {
            char tab_label[64];
            const char *lbl = (i < ntl) ? tlabels[i] : children[i]->label();
            if (!lbl || !*lbl) { snprintf(tab_label, sizeof(tab_label), "Tab %d", i + 1); lbl = tab_label; }
            Fl_Toggle_Button *b = new Fl_Toggle_Button(0, 0, colw - 4, 28);
            b->copy_label(lbl);
            b->callback(SermoSideTabs::tab_cb);
            st->col->add(b);
            st->col->fixed(b, 28);
            children[i]->label("");
            st->wiz->add(children[i]);
        }
        st->resize(0, 0, w, h);
        st->select(0);
        if (tlabels) g_strfreev(tlabels);
        return (GtkWidget *)st;
    }

    Fl_Tabs *tabs = new SermoTabs(0, 0, w, h);
    tabs->begin();

    for (int i = 0; i < nchildren; i++) {
        char tab_label[64];
        const char *lbl = (i < ntl) ? tlabels[i] : children[i]->label();
        if (!lbl || !*lbl) {
            snprintf(tab_label, sizeof(tab_label), "Tab %d", i + 1);
            lbl = tab_label;
        }

        /* Créer un Fl_Group enveloppe pour l'onglet */
        Fl_Group *tab = new Fl_Group(0, 25, w, h - 25, nullptr);
        tab->copy_label(lbl);
        tab->begin();
        children[i]->resize(2, 27, w - 4, h - 29);
        /* Effacer le label du contenu pour éviter le doublon */
        children[i]->label("");
        tab->add(children[i]);
        tab->end();
    }

    tabs->end();
    if (tlabels) g_strfreev(tlabels);
    return (GtkWidget *)tabs;
}

gchar *widget_notebook_envvar_construct(GtkWidget *widget)
{
    if (widget && ((Fl_Widget *)widget)->user_data() == &SERMO_SIDETABS_TAG) {
        char buf[16]; snprintf(buf, sizeof(buf), "%d", ((SermoSideTabs *)widget)->current());
        return g_strdup(buf);
    }
    Fl_Tabs *tabs = (Fl_Tabs *)widget;
    if (!tabs) return g_strdup("0");
    /* Retourner l'index de l'onglet actif (base 0) */
    Fl_Widget *val = tabs->value();
    if (!val) return g_strdup("0");
    for (int i = 0; i < tabs->children(); i++) {
        if (tabs->child(i) == val) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", i);
            return g_strdup(buf);
        }
    }
    return g_strdup("0");
}

gchar *widget_notebook_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_notebook_envvar_construct(var->Widget);
}

void widget_notebook_clear(variable *var)
{
    if (!var || !var->Widget) return;
    if (((Fl_Widget *)var->Widget)->user_data() == &SERMO_SIDETABS_TAG) {
        ((SermoSideTabs *)var->Widget)->select(0); return;
    }
    Fl_Tabs *tabs = (Fl_Tabs *)var->Widget;
    if (tabs->children() > 0)
        tabs->value(tabs->child(0));
}

void widget_notebook_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_notebook_fileselect(variable *var, const char *n, const char *v) {}
void widget_notebook_removeselected(variable *var) {}
void widget_notebook_save(variable *var) {}
