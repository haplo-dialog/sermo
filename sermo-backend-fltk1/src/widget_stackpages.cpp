/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_stackpages.cpp — N pages, une seule visible (FLTK 1.4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Fl_Wizard EST une pile de pages : il n'en montre qu'une. Le nom est trompeur
 * (il ne fabrique aucune navigation) — c'est exactement GtkStack.
 *
 * ⚠️ Le type vit dans un en-tête PARTAGÉ (sermo_stack_pages.h) : <wizard>
 * s'appuiera dessus, et une classe par fichier redonnerait le bogue des deux
 * types de même nom déjà payé sur les échelles.
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
#include "widget_stackpages.h"
#include "sermo_stack_pages.h"
#include <FL/Fl.H>
#include <FL/Fl_Flex.H>
#include <FL/Fl_Button.H>
#include <string.h>
#include <stdlib.h>

static void page_choisie(Fl_Widget *b, void *donnee)
{
    SermoPages *pages = (SermoPages *) donnee;
    if (pages) pages->aller_a((int)(intptr_t) b->user_data());
}

GtkWidget *widget_stackpages_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    int page = 0, avec_switcher = 0;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "page"))) page = atoi(v);
        if ((v = get_tag_attribute(attr, "switcher")))
            avec_switcher = (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || atoi(v) == 1);
    }

    SermoPages *pages = new SermoPages(0, 0, 800, 400);
    pages->end();

    stackelement s = pop();
    int n = 0, haut = 0;
    for (int i = 0; i < s.nwidgets; ++i) {
        Fl_Widget *c = (Fl_Widget *) s.widgets[i];
        if (!c) continue;
        c->resize(0, 0, 800, c->h() > 0 ? c->h() : 200);
        pages->add(c);
        if (c->h() > haut) haut = c->h();
        n++;
    }
    if (haut > 0) pages->size(800, haut);
    if (page < 0) page = 0;
    if (page >= n && n > 0) page = n - 1;
    pages->aller_a(page);

    if (!avec_switcher) return (GtkWidget *) pages;

    Fl_Flex *hote  = new Fl_Flex(0, 0, 800, haut + 34, Fl_Flex::COLUMN);
    hote->end();
    Fl_Flex *barre = new Fl_Flex(0, 0, 800, 30, Fl_Flex::ROW);
    barre->end();
    for (int i = 0; i < n; ++i) {
        char t[8]; snprintf(t, sizeof(t), "%d", i + 1);
        Fl_Button *b = new Fl_Button(0, 0, 40, 28);
        b->copy_label(t);
        b->user_data((void *)(intptr_t) i);
        b->callback(page_choisie, pages);
        barre->add(b);
        barre->fixed(b, 40);
    }
    hote->add(barre); hote->fixed(barre, 30);
    hote->add(pages);
    hote->user_data(pages);   /* l'export retrouve la pile par là */
    return (GtkWidget *) hote;
}

/* Export : l'index de la page visible, comme <notebook> (étalon gtk3). */
gchar *widget_stackpages_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("0");
    Fl_Widget  *w = (Fl_Widget *) widget;
    SermoPages *pages = dynamic_cast<SermoPages *>(w);
    if (!pages) pages = (SermoPages *) w->user_data();   /* enveloppe */
    if (!pages) return g_strdup("0");
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", pages->page());
    return g_strdup(buf);
}
gchar *widget_stackpages_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_stackpages_envvar_construct(var->Widget);
}
void widget_stackpages_clear(variable *var)   { (void) var; }
void widget_stackpages_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *) var->Widget)->redraw();
}
void widget_stackpages_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_stackpages_removeselected(variable *var) { (void) var; }
void widget_stackpages_save(variable *var)           { (void) var; }
