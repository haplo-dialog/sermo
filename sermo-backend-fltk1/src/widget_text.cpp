/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_text.cpp — Texte en lecture seule FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <text> → Fl_Output (mono-ligne, read-only, pas de bordure visible)
 * Le label XML est affiché tel quel. Supporte l'attribut justify.
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
#include "widget_text.h"

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_text_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 200, h = 22;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Output *out = new Fl_Output(0, 0, w, h, nullptr);
    out->box(FL_NO_BOX);  /* pas de bordure — style label */

    /* Label <label>texte</label> */
    if (Attr) {
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) out->value(lbl);
    }

    return (GtkWidget *)out;
}

gchar *widget_text_envvar_construct(GtkWidget *widget)
{
    Fl_Output *out = (Fl_Output *)widget;
    if (!out || !out->value()) return g_strdup("");
    return g_strdup(out->value());
}

gchar *widget_text_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_text_envvar_construct(var->Widget);
}

void widget_text_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((Fl_Output *)var->Widget)->value("");
}

void widget_text_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    /* <input> (commande ou fichier) : le contenu ENTIER, saut de ligne final
     * compris — règle de l'étalon gtk3sermo. Jusqu'à la 2.7.0, <text> ignorait
     * <input>. Le cœur appelle cette fonction juste après la création. */
    gchar *text = widget_input_text(var->Attributes);
    if (text) {
        ((Fl_Output *)var->Widget)->value(text);
        g_free(text);
    }
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_text_fileselect(variable *var, const char *n, const char *v) {}
void widget_text_removeselected(variable *var) {}
void widget_text_save(variable *var) {}
