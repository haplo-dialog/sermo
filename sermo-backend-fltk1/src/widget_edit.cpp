/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_edit.cpp — Zone de texte multi-lignes éditable FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <edit> → Fl_Multiline_Input (éditable)
 * <input> (commande ou fichier) alimente le contenu ENTIER, saut de ligne
 * final compris (étalon gtk3sermo) ; lu dans refresh seulement.
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
#include "widget_edit.h"
#include "safe_exec.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_edit_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 400, h = 200;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Multiline_Input *inp = new Fl_Multiline_Input(0, 0, w, h, nullptr);
    /* background / foreground (extension sermo) : #rrggbb */
    if (attr) {
        const char *bg = get_tag_attribute(attr, "background");
        const char *fg = get_tag_attribute(attr, "foreground");
        unsigned v;
        if (bg && bg[0] == '#' && sscanf(bg + 1, "%6x", &v) == 1)
            inp->color(fl_rgb_color((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff));
        if (fg && fg[0] == '#' && sscanf(fg + 1, "%6x", &v) == 1) {
            inp->textcolor(fl_rgb_color((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff));
            inp->cursor_color(inp->textcolor());
        }
    }
    inp->box(FL_DOWN_BOX);

    /* Valeur statique. <input> est lu dans refresh, que le cœur appelle juste
     * après la création : lu ici aussi, la commande tournait deux fois. Il
     * remplace <default> s'il rend quelque chose, comme avant. */
    if (Attr) {
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) inp->value(def);
    }

    return (GtkWidget *)inp;
}

gchar *widget_edit_envvar_construct(GtkWidget *widget)
{
    Fl_Multiline_Input *inp = (Fl_Multiline_Input *)widget;
    if (!inp || !inp->value()) return g_strdup("");
    return g_strdup(inp->value());
}

gchar *widget_edit_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_edit_envvar_construct(var->Widget);
}

void widget_edit_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Multiline_Input *)var->Widget)->value("");
}

void widget_edit_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    /* Re-résoudre <input> (Command:/file: décodés par le helper) */
    Fl_Multiline_Input *inp = (Fl_Multiline_Input *)var->Widget;
    gchar *text = widget_input_text(var->Attributes);
    if (text) {
        inp->value(text);
        g_free(text);
        inp->redraw();
    }
}

void widget_edit_fileselect(variable *var, const char *n, const char *v) {}
void widget_edit_removeselected(variable *var) {}
void widget_edit_save(variable *var) {}
