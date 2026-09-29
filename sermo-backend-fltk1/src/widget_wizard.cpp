/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_wizard.cpp — Suite d'étapes (FLTK 1.4)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Fl_Wizard porte le nom mais ne fabrique AUCUNE navigation : c'est une pile
 * de pages. Les trois boutons sont donc ajoutés ici, comme sur les six autres
 * ports. Le type de pile vient de l'en-tête partagé (leçon des échelles).
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
#include "widget_wizard.h"
#include "sermo_stack_pages.h"
#include <FL/Fl.H>
#include <FL/Fl_Flex.H>
#include <FL/Fl_Button.H>
#include <string.h>
#include <stdlib.h>

/* execute_action vient de actions.h : une seule déclaration, sinon violation
 * de l'ODR (type de retour et constance divergents). */
#include "actions.h"

static void wiz_precedent(Fl_Widget *, void *d)
{
    SermoPages *p = (SermoPages *) d;
    if (p) p->aller_a(p->page() - 1);
}
static void wiz_suivant(Fl_Widget *, void *d)
{
    SermoPages *p = (SermoPages *) d;
    if (p) p->aller_a(p->page() + 1);
}
static void wiz_terminer(Fl_Widget *b, void *d)
{
    AttributeSet *Attr = (AttributeSet *) d;
    if (!Attr) return;
    GList *el = NULL;
    gchar *cmd = attributeset_get_first(&el, Attr, ATTR_ACTION);
    while (cmd) {
        if (*cmd) execute_action((GtkWidget *) b, cmd, NULL);
        cmd = attributeset_get_next(&el, Attr, ATTR_ACTION);
    }
}

GtkWidget *widget_wizard_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) attr; (void) Type;

    SermoPages *pages = new SermoPages(0, 0, 800, 300);
    pages->end();

    stackelement s = pop();
    int haut = 0;
    for (int i = 0; i < s.nwidgets; ++i) {
        Fl_Widget *c = (Fl_Widget *) s.widgets[i];
        if (!c) continue;
        c->resize(0, 0, 800, c->h() > 0 ? c->h() : 160);
        pages->add(c);
        if (c->h() > haut) haut = c->h();
    }
    if (haut > 0) pages->size(800, haut);
    pages->aller_a(0);

    Fl_Flex *hote  = new Fl_Flex(0, 0, 800, haut + 34, Fl_Flex::COLUMN);
    hote->end();
    Fl_Flex *barre = new Fl_Flex(0, 0, 800, 30, Fl_Flex::ROW);
    barre->end();

    Fl_Button *prec = new Fl_Button(0, 0, 100, 28); prec->copy_label("Précédent");
    Fl_Button *suiv = new Fl_Button(0, 0, 100, 28); suiv->copy_label("Suivant");
    Fl_Button *fin  = new Fl_Button(0, 0, 100, 28); fin->copy_label("Terminer");
    prec->callback(wiz_precedent, pages);
    suiv->callback(wiz_suivant, pages);
    fin->callback(wiz_terminer, Attr);
    barre->add(prec); barre->fixed(prec, 100);
    barre->add(suiv); barre->fixed(suiv, 100);
    barre->add(fin);  barre->fixed(fin, 100);

    hote->add(pages);
    hote->add(barre); hote->fixed(barre, 30);
    hote->user_data(pages);   /* l'export retrouve la pile par là */
    return (GtkWidget *) hote;
}

/* Export : l'index de l'étape courante, comme <stack>. */
gchar *widget_wizard_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("0");
    SermoPages *p = (SermoPages *) ((Fl_Widget *) widget)->user_data();
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", p ? p->page() : 0);
    return g_strdup(buf);
}
gchar *widget_wizard_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_wizard_envvar_construct(var->Widget);
}
void widget_wizard_clear(variable *var)   { (void) var; }
void widget_wizard_refresh(variable *var)
{
    if (var && var->Widget) ((Fl_Widget *) var->Widget)->redraw();
}
void widget_wizard_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_wizard_removeselected(variable *var) { (void) var; }
void widget_wizard_save(variable *var)           { (void) var; }
