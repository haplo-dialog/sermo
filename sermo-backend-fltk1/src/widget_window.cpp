/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_window.cpp — Fenêtre principale FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Architecture :
 *   Le core fltk1dialog (C) parse le XML et appelle widget_window_create().
 *   On retourne un Fl_Double_Window* casté en GtkWidget* (= Fl_Widget*).
 *   Les enfants ont déjà été pushés sur la pile par le parser ; on les
 *   récupère via pop() et on les ajoute à la fenêtre.
 *
 *   Disposition : Fl_Pack vertical occupe toute la fenêtre.
 *   L'enfant unique du <window> (toujours un <vbox> ou <hbox>) est
 *   ajouté à ce pack.
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
#include "widget_window.h"
#include "sermo_icon_theme.h"
#include <FL/Fl_PNG_Image.H>

#include <string.h>
#include <stdlib.h>

/* Callback fermeture fenêtre → quitter le programme */
static void window_close_cb(Fl_Widget *w, void *)
{
    w->hide();
}

/* ── widget_window_clear ─────────────────────────────────────────────────── */
void widget_window_clear(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Double_Window *win = (Fl_Double_Window *)var->Widget;
    win->label("fltk1dialog");
}

/* ── widget_window_create ────────────────────────────────────────────────── */
GtkWidget *widget_window_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList   *element  = NULL;
    gchar   *value    = NULL;
    int      dw       = 640;
    int      dh       = 480;
    int      resizable = 1;
    const char *title = "fltk1dialog";

    /* Titre : attribut de balise <window title="..."> d'abord (comme les
     * autres ports), repli sur ATTR_LABEL. */
    if (attr && (value = get_tag_attribute(attr, "title")) && *value)
        title = value;
    else if (Attr) {
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) title = lbl;
    }

    /* Taille demandée : width/height-request prime, puis default-width/height,
     * sinon la fenêtre s'adaptera au CONTENU (comme l'étalon gtk3). */
    int explicit_w = 0, explicit_h = 0;
    if (attr) {
        if ((value = get_tag_attribute(attr, "default-width")))  { dw = atoi(value); explicit_w = 1; }
        if ((value = get_tag_attribute(attr, "default-height"))) { dh = atoi(value); explicit_h = 1; }
        if ((value = get_tag_attribute(attr, "width-request")))  { dw = atoi(value); explicit_w = 1; }
        if ((value = get_tag_attribute(attr, "height-request"))) { dh = atoi(value); explicit_h = 1; }
        if ((value = get_tag_attribute(attr, "resizable"))) {
            if (strcasecmp(value, "false") == 0 ||
                strcasecmp(value, "no")    == 0 ||
                strcmp(value, "0")         == 0)
                resizable = 0;
        }
    }

    /* Créer la fenêtre */
    Fl_Double_Window *win = new Fl_Double_Window(dw, dh, title);
    win->callback(window_close_cb, nullptr);
    /* Icone de fenetre (barre de titre) = icone d'appli du port, comme
     * gtk/qt. Cherchee dans le theme (fltk1sermo, puis fltk1dialog). */
    {
        char *ip = sermo_icon_lookup("fltk1sermo", 32);
        if (!ip) ip = sermo_icon_lookup("fltk1dialog", 32);
        if (ip) {
            Fl_RGB_Image *ic = new Fl_PNG_Image(ip);
            if (ic && ic->w() > 0) win->icon(ic);
            free(ip);
        }
    }
    if (!resizable) win->resizable((Fl_Widget *)nullptr);
    win->end();    /* le constructeur fait begin() : fermer tout de suite */

    /* Adopter l'enfant dépilé (vbox/hbox). L'ancien begin()/end() n'adoptait
     * rien : l'enfant existait déjà — il faut add(), pas begin(). C'était la
     * cause des fenêtres vides. */
    stackelement s = pop();
    Fl_Widget *child0 = (s.nwidgets > 0) ? (Fl_Widget *)s.widgets[0] : NULL;

    /* Sans taille explicite : adapter la fenêtre au contenu du pack
     * (parité étalon — gtk3 rétrécit sa fenêtre au contenu ; à 640x480,
     * un « clic en bas de fenêtre » des bancs tombe dans le vide). */
    if ((!explicit_w || !explicit_h) && child0 &&
        child0->user_data() == &SERMO_FLEX_TAG) {
        Fl_Flex *fx = (Fl_Flex *)child0;   /* vbox/hbox = Fl_Flex du port */
        int n = fx->children(), tw = 0, th = 0;
        int vertical = !fx->horizontal();
        for (int i = 0; i < n; i++) {
            Fl_Widget *c = fx->child(i);
            if (vertical) { th += c->h(); if (c->w() > tw) tw = c->w(); }
            else          { tw += c->w(); if (c->h() > th) th = c->h(); }
        }
        if (n > 1) { if (vertical) th += fx->gap() * (n - 1);
                     else          tw += fx->gap() * (n - 1); }
        th += 4; tw += 4;   /* marges du flex */
        if (!explicit_w && tw > 0) dw = tw;
        if (!explicit_h && th > 0) dh = th;
        win->size(dw, dh);
    }

    for (int n = 0; n < s.nwidgets; n++) {
        Fl_Widget *child = (Fl_Widget *)s.widgets[n];
        if (!child) continue;
        child->resize(0, 0, dw, dh);
        win->add(child);
    }

    /* Centrage si demandé */
    if (attr) {
        if ((value = get_tag_attribute(attr, "window-position"))) {
            int pos = atoi(value);
            if (pos == 1) { /* GTK_WIN_POS_CENTER */
                int sx = Fl::w();
                int sy = Fl::h();
                win->position((sx - dw) / 2, (sy - dh) / 2);
            }
        }
    }

    return (GtkWidget *)win;
}

/* ── widget_window_envvar_construct ─────────────────────────────────────── */
gchar *widget_window_envvar_construct(GtkWidget *widget)
{
    /* Comme dans GTK : les fenêtres n'exportent pas de variable */
    return NULL;
}

gchar *widget_window_envvar_all_construct(variable *var)
{
    return NULL;
}

/* ── widget_window_refresh ───────────────────────────────────────────────── */
void widget_window_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Double_Window *)var->Widget)->redraw();
}

/* ── Stubs non applicables ───────────────────────────────────────────────── */
void widget_window_fileselect(variable *var, const char *name, const char *value)
{
    g_warning("%s(): not implemented", __func__);
}

void widget_window_removeselected(variable *var)
{
    g_warning("%s(): not implemented", __func__);
}

void widget_window_save(variable *var)
{
    g_warning("%s(): not implemented", __func__);
}
