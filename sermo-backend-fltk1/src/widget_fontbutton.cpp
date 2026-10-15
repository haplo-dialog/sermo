/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_fontbutton.cpp — Bouton de sélection de police FLTK
 * sermo — haplo-dialog — GPL-2.0-or-later
 * Implémenté via safe_popen("fc-list : family") + Fl_Choice.
 * Export : nom de police sélectionnée. */
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
#include "widget_fontbutton.h"
#include <FL/Fl_Choice.H>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const int FONT_MAX = 256;

/* Fl_Choice + repère du premier passage (faux jusqu'au premier refresh).
 * Membre du widget : rien à libérer. */
struct FontChoice : Fl_Choice {
    bool lu;
    FontChoice(int X, int Y, int W, int H) : Fl_Choice(X, Y, W, H), lu(false) {}
};

GtkWidget *widget_fontbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Fl_Choice *ch = new FontChoice(0, 0, 200, 30);

    /* Le <default> ("Sans 12") est la valeur de VÉRITÉ : posé en tête et
     * sélectionné tel quel — parité étalon, qui rend la chaîne de police
     * telle que fournie. */
    gchar *def = NULL;
    if (Attr) {
        GList *el = NULL;
        def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
    }
    ch->add(def && *def ? def : "Sans");

    /* Familles système par l'API NATIVE FLTK (Fl::set_fonts) : zéro
     * sous-processus — l'ancien fc-list coûtait un spawn par création
     * (et sa première version à tubes déclenchait le repli shell). */
    {
        int nf = Fl::set_fonts(NULL);
        int n = 0;
        for (int i = 0; i < nf && n < FONT_MAX; i++) {
            int attrs = 0;
            const char *name = Fl::get_font_name((Fl_Font)i, &attrs);
            if (name && *name && !attrs && !ch->find_item(name)) {
                ch->add(name);
                ++n;
            }
        }
    }
    ch->value(0);
    return (GtkWidget *)ch;
}
gchar *widget_fontbutton_envvar_construct(GtkWidget *w)
{
    Fl_Choice *c = (Fl_Choice *)w;
    const Fl_Menu_Item *it = c->mvalue();
    return g_strdup(it && it->label() ? it->label() : "Sans");
}
gchar *widget_fontbutton_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_fontbutton_envvar_construct(v->Widget) : NULL; }
void widget_fontbutton_clear(variable *v)
{ if (v && v->Widget) ((Fl_Choice *)v->Widget)->value(0); }
/* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo. Jusqu'au
 * 2026-10-13 il n'était jamais lu. La première ligne est le nom de police
 * (« Sans 13 ») : choisie si la liste l'a, sinon ajoutée puis choisie — la
 * valeur exportée est la chaîne telle quelle, comme chez l'étalon. Posée sans
 * jouer d'action (value() de FLTK n'appelle pas le rappel). */
void widget_fontbutton_refresh(variable *v)
{
    if (!v || !v->Widget) return;
    Fl_Choice *ch = (Fl_Choice *)v->Widget;
    FontChoice *fc = (FontChoice *)v->Widget;
    bool premier = !fc->lu;
    fc->lu = true;
    gchar *ligne = widget_input_line(v->Attributes);
    /* Premier passage (2026-10-13) : l'étalon pose l'<input> PUIS le <default> ;
     * un <default> l'emporte donc (la commande a tourné quand même). La
     * création l'a mis en tête de liste et choisi : on n'y touche pas. */
    if (ligne && premier && v->Attributes) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, v->Attributes, ATTR_DEFAULT);
        if (def && *def) { g_free(ligne); ligne = NULL; }
    }
    if (ligne) {
        int trouve = -1;
        for (int i = 0; i < ch->size(); i++) {
            const char *l = ch->menu()[i].label();
            if (l && strcmp(l, ligne) == 0) { trouve = i; break; }
        }
        if (trouve < 0 && *ligne) {
            /* add() lit « / » comme un sous-menu et « \\ » comme un échappement :
             * on les échappe pour que l'élément porte la chaîne exacte. */
            GString *e = g_string_new(NULL);
            for (const char *c = ligne; *c; c++) {
                if (*c == '/' || *c == '\\') g_string_append_c(e, '\\');
                g_string_append_c(e, *c);
            }
            trouve = ch->add(e->str);
            g_string_free(e, TRUE);
        }
        if (trouve >= 0) ch->value(trouve);
        g_free(ligne);
    }
    ch->redraw();
}
void widget_fontbutton_fileselect(variable *v, const char*, const char*) {}
void widget_fontbutton_removeselected(variable *v) {}
void widget_fontbutton_save(variable *v) {}
