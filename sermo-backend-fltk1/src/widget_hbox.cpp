/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_hbox.cpp — Conteneur horizontal FLTK (Fl_Pack horizontal)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_hbox.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Input_.H>
#include <FL/Fl_Button.H>

#include <stdlib.h>
#include <stdio.h>
#include <vector>

/* Rangée <hbox> : règle de l'étalon gtk3 (sermo-backend-gtk3/src/widget_hbox.c).
 *  - (expand, fill) courant = celui de la BOÎTE (ou « non dit ») ; les enfants
 *    sont parcourus À L'ENVERS (gtk_box_pack_end) et un enfant qui déclare le
 *    sien le remplace pour lui ET pour ceux parcourus après lui ;
 *  - « non dit » : <edit>, <frame>, fenêtre défilante, <entry> s'étirent ;
 *  - ce qui ne s'étire pas se cale à DROITE ; un enfant étiré sans fill est
 *    centré dans sa case ; homogeneous = cases égales sur toute la largeur.
 * La répartition elle-même est faite par SermoFlex (widgets.cpp). */
GtkWidget *widget_hbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    SermoFlex *flex = new SermoFlex(0, 0, 800, 30, Fl_Flex::ROW);
    flex->end();
    int spacing = 4;
    if (attr) {
        const char *v = get_tag_attribute(attr, "spacing");
        if (v) spacing = atoi(v);
    }
    flex->gap(spacing);
    flex->user_data(&SERMO_FLEX_TAG);
    flex->pack_end(true);
    flex->homogeneous(sermo_attr_rule(attr, "homogeneous", 0) == 1);

    stackelement s = pop();
    /* Hauteur de la rangée : la plus grande hauteur NATURELLE des enfants, lue
     * AVANT de les ajouter — Fl_Flex (créé à 30 px) les remet à sa propre
     * hauteur dès l'ajout, et une colonne image + texte débordait alors sous
     * la rangée (galerie « Boutons et icônes », jusqu'à la 2.8.2). */
    int hmax = 0;
    for (int n = 0; n < s.nwidgets; n++)
        if (s.widgets[n] && ((Fl_Widget *)s.widgets[n])->h() > hmax) hmax = ((Fl_Widget *)s.widgets[n])->h();

    /* règles (expand, fill), parcours à l'envers comme gtk_box_pack_end */
    int cur_e = sermo_attr_rule(attr, "space-expand", project_space_expand);
    int cur_f = sermo_attr_rule(attr, "space-fill", project_space_fill);
    std::vector<char> ex(s.nwidgets + 1, 0), fi(s.nwidgets + 1, 0);
    std::vector<int>  nh(s.nwidgets + 1, 0);
    for (int n = s.nwidgets - 1; n >= 0; --n) {
        Fl_Widget *c = (Fl_Widget *)s.widgets[n];
        ex[n] = fi[n] = false; nh[n] = 0;
        if (!c) continue;
        int e = sermo_widget_expand_rule(c), f = sermo_widget_fill_rule(c);
        if (e != -1) cur_e = e;
        if (f != -1) cur_f = f;
        int t = s.widgettypes[n];
        bool def = (t == WIDGET_EDIT || t == WIDGET_FRAME ||
                    t == WIDGET_SCROLLEDW || t == WIDGET_ENTRY);
        ex[n] = cur_e != -1 ? cur_e == 1 : def;
        fi[n] = cur_f != -1 ? cur_f == 1 : def;
        nh[n] = c->h();
    }

    int natw = 0, added = 0;
    for (int n = 0; n < s.nwidgets; n++) {
        Fl_Widget *c = (Fl_Widget *)s.widgets[n];
        if (!c) continue;
        int nat = sermo_natural_w(c);
        flex->add(c);
        flex->child_rule(c, ex[n], fi[n], nat, nh[n], sermo_can_grow_v(c));
        natw += nat;
        added++;
    }
    if (added > 1) natw += spacing * (added - 1);
    flex->natural_w(natw);
    if (hmax > 0) flex->size(800, hmax);
    return (GtkWidget *)flex;
}

void widget_hbox_clear(variable *var) {}
gchar *widget_hbox_envvar_construct(GtkWidget *w) { return NULL; }
gchar *widget_hbox_envvar_all_construct(variable *var) { return NULL; }
void widget_hbox_refresh(variable *var) { if (var && var->Widget) ((Fl_Widget *)var->Widget)->redraw(); }
void widget_hbox_fileselect(variable *var, const char *n, const char *v) {}
void widget_hbox_removeselected(variable *var) {}
void widget_hbox_save(variable *var) {}
