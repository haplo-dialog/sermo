/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_menubar.cpp — Barre de menus FLTK bâtie depuis l'arbre <menubar>/
 * <menu>/<menuitem> (carriers dépilés). Hiérarchie + icônes de thème
 * (Fl_Multi_Label) + cases à cocher. GPL-2.0-or-later
 *
 * Sécurité : actions routées par execute_action (exit:/refresh:/shell via
 * safe_system) — jamais system()/popen() directs.
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
#include "stack.h"
#include "safe_exec.h"
#include "actions.h"
#include "widget_menubar.h"
#include "sermo_menu_model.h"
#include "sermo_icon_theme.h"

#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Multi_Label.H>
#include <FL/Fl_Shared_Image.H>

#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>

extern "C" void action_exitprogram(GtkWidget *widget, char *string);

/* Données persistantes attachées au widget (libellés, commandes, multi-labels
 * — pointés par le tableau Fl_Menu_Item copié). */
struct MenuBarData {
    std::vector<char *>           cmds;
    std::vector<char *>           texts;    /* libellés — Fl_Menu_::copy ne les duplique PAS */
    std::vector<Fl_Multi_Label *> mlabels;
};

/* Clic sur un item : router la commande par le répartiteur du cœur. */
static void _menu_cb(Fl_Widget *w, void *ud)
{
    const char *cmd = (const char *)ud;
    if (cmd && *cmd) execute_action((GtkWidget *)w, cmd, NULL);
}

/* échappe les '/' d'un libellé feuille pour Fl_Menu_Bar (qui les prend pour
 * des séparateurs de hiérarchie). */
static std::string esc(const std::string &s)
{
    std::string o;
    for (char ch : s) { if (ch == '/') o += '\\'; o += ch; }
    return o;
}

GtkWidget *widget_menubar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int win_w = 800;
    if (attr) {
        const char *v = get_tag_attribute(attr, "width-request");
        if (v && *v) win_w = atoi(v);
    }

    /* Dépiler les <menu> (SUM, ordre document). */
    stackelement s = pop();

    Fl_Menu_Bar *mb = new Fl_Menu_Bar(0, 0, win_w, 25);
    mb->box(FL_FLAT_BOX);
    MenuBarData *d = new MenuBarData();

    /* Construire le tableau Fl_Menu_Item À LA MAIN (hiérarchie par
     * FL_SUBMENU + terminateurs) puis copy(). ⚠️ Fl_Menu_::copy ne duplique
     * PAS les chaînes de texte : elles doivent survivre à cette fonction →
     * strdup dans d->texts (persistant avec le widget). */
    std::vector<Fl_Menu_Item> arr;
    struct IconPost { int idx; std::string icon; };
    std::vector<IconPost> posts;

    auto put = [&](const std::string &txt, int flags, Fl_Callback *cb, void *ud) -> int {
        char *t = strdup(txt.c_str());
        d->texts.push_back(t);
        Fl_Menu_Item it; memset(&it, 0, sizeof(it));
        it.text = t;
        it.flags = flags;
        it.callback_ = cb;
        it.user_data_ = ud;
        it.labelfont_ = FL_HELVETICA;
        it.labelsize_ = 14;
        it.labelcolor_ = FL_FOREGROUND_COLOR;
        arr.push_back(it);
        return (int)arr.size() - 1;
    };
    auto put_null = [&]() { Fl_Menu_Item it; memset(&it, 0, sizeof(it)); arr.push_back(it); };

    for (int n = 0; n < s.nwidgets; n++) {
        Fl_Widget *w = (Fl_Widget *)s.widgets[n];
        if (!w) continue;
        SermoMenuCarrier *mc = (SermoMenuCarrier *)w->user_data();
        if (!mc || mc->kind != 1) continue;

        std::string mlabel = mc->menu.label.empty() ? "Menu" : mc->menu.label;
        put(esc(mlabel), FL_SUBMENU, 0, 0);
        for (size_t k = 0; k < mc->menu.items.size(); k++) {
            SermoMenuItem &it = mc->menu.items[k];
            if (it.separator) {
                /* diviseur : marquer l'item précédent (s'il existe dans ce sous-menu) */
                if (!arr.empty() && !(arr.back().flags & FL_SUBMENU) && arr.back().text)
                    arr.back().flags |= FL_MENU_DIVIDER;
                continue;
            }
            int flags = 0;
            if (it.has_check) { flags |= FL_MENU_TOGGLE; if (it.check_val) flags |= FL_MENU_VALUE; }
            char *cmd = it.cmd.empty() ? nullptr : strdup(it.cmd.c_str());
            if (cmd) d->cmds.push_back(cmd);
            std::string leaf = it.label.empty() ? "(vide)" : it.label;
            int idx = put(esc(leaf), flags, cmd ? _menu_cb : 0, cmd);
            if (!it.icon.empty()) posts.push_back({idx, it.icon});
        }
        put_null();   /* ferme le sous-menu */
    }
    put_null();       /* fin du tableau */

    mb->copy(arr.data());

    /* Icônes de thème : après copy() (tableau interne stable), remplacer le
     * label des items concernés par un Fl_Multi_Label image+texte. */
    for (size_t i = 0; i < posts.size(); i++) {
        char *ip = sermo_icon_lookup(posts[i].icon.c_str(), sermo_icon_size_px("menu"));
        if (!ip) continue;
        Fl_Shared_Image *img = Fl_Shared_Image::get(ip, 16, 16);
        free(ip);
        if (!img) continue;
        Fl_Menu_Item *mi = (Fl_Menu_Item *)&mb->menu()[posts[i].idx];
        Fl_Multi_Label *ml = new Fl_Multi_Label();
        ml->typea = FL_IMAGE_LABEL;  ml->labela = (const char *)img;
        ml->typeb = FL_NORMAL_LABEL; ml->labelb = mi->text;   /* copie stable FLTK */
        d->mlabels.push_back(ml);
        ml->label(mi);   /* pose _FL_MULTI_LABEL, garde le texte via labelb */
    }

    mb->user_data(d);
    return (GtkWidget *)mb;
}

extern "C" gchar *widget_menubar_envvar_construct(GtkWidget *)      { return NULL; }
extern "C" gchar *widget_menubar_envvar_all_construct(variable *)   { return NULL; }
extern "C" void widget_menubar_clear(variable *)                    {}
extern "C" void widget_menubar_refresh(variable *var)               { if (var && var->Widget) ((Fl_Widget *)var->Widget)->redraw(); }
extern "C" void widget_menubar_fileselect(variable *, const char *, const char *) {}
extern "C" void widget_menubar_removeselected(variable *)           {}
extern "C" void widget_menubar_save(variable *)                     {}
