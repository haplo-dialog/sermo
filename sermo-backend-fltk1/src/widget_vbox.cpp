/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_vbox.cpp — Conteneur vertical FLTK (Fl_Pack vertical)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <vbox> → Fl_Pack(VERTICAL) : empile ses enfants verticalement.
 * Les enfants ont été pushés sur la pile par le parser (LIFO) ; on les
 * dépile dans l'ordre inverse pour les ajouter dans le bon sens.
 *
 * Taille : le pack se dimensionne sur son parent à la réalisation.
 *          On fixe une taille initiale généreuse ; Fl_Pack recalcule.
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
#include "widget_vbox.h"
#include <FL/Fl_Browser_.H>
#include <FL/Fl_Text_Display.H>

#include <stdlib.h>
#include <string.h>

/* ── Helpers partagés avec hbox ──────────────────────────────────────────── */

/* Crée un Fl_Pack avec l'orientation donnée et y place les enfants de la pile */
static Fl_Pack *fltk_pack_create(AttributeSet *Attr, tag_attr *attr,
                                  int orientation)
{
    int spacing = 2;
    if (attr) {
        const char *v = get_tag_attribute(attr, "spacing");
        if (v) spacing = atoi(v);
    }

    /* Taille initiale généreuse — sera recalculée par le parent */
    Fl_Pack *pack = new Fl_Pack(0, 0, 800, 600);
    pack->type(orientation);
    pack->spacing(spacing);

    /* Dépiler les enfants (le dernier empilé = premier à afficher) */
    pack->begin();
    stackelement s;
    while ((s = pop()).widgets[0] != nullptr) {
        Fl_Widget *child = (Fl_Widget *)s.widgets[0];
        /* Les enfants de type feuille ont déjà leur taille ; les packs
         * enfants se redimensionneront récursivement */
        (void)child; /* déjà dans pack via begin/end */
        if (s.widgettypes[0] == WIDGET_VBOX ||
            s.widgettypes[0] == WIDGET_HBOX ||
            s.widgettypes[0] == WIDGET_FRAME ||
            s.widgettypes[0] == WIDGET_NOTEBOOK ||
            s.widgettypes[0] == WIDGET_WINDOW)
            break; /* ne pas traverser les conteneurs parents */
    }
    pack->end();

    return pack;
}

/* ── widget_vbox_create ──────────────────────────────────────────────────── */
GtkWidget *widget_vbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    /* Fl_Flex (FLTK 1.4) au lieu de Fl_Pack : les enfants sont VRAIMENT
     * repartis dans la largeur ; en colonne chacun garde sa hauteur
     * naturelle (fixed). Fl_Pack empilait des tailles brutes qui se
     * chevauchaient des que le contenu depassait. */
    Fl_Flex *flex = new Fl_Flex(0, 0, 800, 600, Fl_Flex::COLUMN);
    flex->end();
    int spacing = 4;
    if (attr) {
        const char *v = get_tag_attribute(attr, "spacing");
        if (v) spacing = atoi(v);
    }
    flex->gap(spacing);
    flex->margin(2, 2);
    flex->user_data(&SERMO_FLEX_TAG);

    stackelement s = pop();
    int total = 0, added = 0;
    for (int n = 0; n < s.nwidgets; n++) {
        Fl_Widget *c = (Fl_Widget *)s.widgets[n];
        if (!c) continue;
        flex->add(c);
        /* seuls les conteneurs et les zones (edit, listes, arbres, tables,
         * defilement) grandissent en hauteur ; un bouton ou une etiquette
         * garde la sienne meme avec space-expand (rendu etalon) */
        bool grows = sermo_widget_expands(c) &&
                     (c->as_group() || dynamic_cast<Fl_Text_Display *>(c) ||
                      dynamic_cast<Fl_Browser_ *>(c));
        if (!grows) flex->fixed(c, c->h());
        total += c->h();
        added++;
    }
    /* hauteur naturelle : sans cela une vbox imbriquee (colonne d'un hbox,
     * page d'onglet) gardait 600 px et poussait le reste hors fenetre */
    if (added > 0) {
        total += spacing * (added - 1) + 4;
        int wreq = 800;
        const char *wr = attr ? get_tag_attribute(attr, "width-request") : NULL;
        if (wr && atoi(wr) > 0) { wreq = atoi(wr); flex->user_data(&SERMO_FLEX_TAG); }
        flex->size(wreq, total);
    }
    return (GtkWidget *)flex;
}

/* ── Stubs ───────────────────────────────────────────────────────────────── */
void widget_vbox_clear(variable *var) {}
gchar *widget_vbox_envvar_construct(GtkWidget *w) { return NULL; }
gchar *widget_vbox_envvar_all_construct(variable *var) { return NULL; }
void widget_vbox_refresh(variable *var) { if (var && var->Widget) ((Fl_Widget *)var->Widget)->redraw(); }
void widget_vbox_fileselect(variable *var, const char *n, const char *v) {}
void widget_vbox_removeselected(variable *var) {}
void widget_vbox_save(variable *var) {}
