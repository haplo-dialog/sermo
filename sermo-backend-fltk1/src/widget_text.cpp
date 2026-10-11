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

#include <string>
#include <string.h>
#include <stdlib.h>

/* Une ligne d'Fl_Output affiche un saut de ligne « ^J ». L'écran montre le
 * texte sans ses sauts de ligne finaux ; la variable garde le texte ENTIER
 * (règle de l'étalon), retenu ici tant que l'écran n'a pas changé. */
class TexteOut : public Fl_Output {
public:
    std::string entier;
    TexteOut(int x, int y, int w, int h) : Fl_Output(x, y, w, h, nullptr) {}
    void poser(const char *t) {
        entier = t ? t : "";
        std::string vu = entier;
        while (!vu.empty() && (vu.back() == '\n' || vu.back() == '\r')) vu.pop_back();
        value(vu.c_str());
    }
};

GtkWidget *widget_text_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 200, h = 22;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    TexteOut *out = new TexteOut(0, 0, w, h);
    out->box(FL_NO_BOX);  /* pas de bordure — style label */

    /* Label <label>texte</label> */
    if (Attr) {
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) out->poser(lbl);
    }

    return (GtkWidget *)out;
}

gchar *widget_text_envvar_construct(GtkWidget *widget)
{
    TexteOut *out = (TexteOut *)widget;
    if (!out || !out->value()) return g_strdup("");
    std::string vu = out->entier;
    while (!vu.empty() && (vu.back() == '\n' || vu.back() == '\r')) vu.pop_back();
    if (vu == out->value()) return g_strdup(out->entier.c_str());   /* écran inchangé */
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
    ((TexteOut *)var->Widget)->poser("");
}

void widget_text_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    /* <input> (commande ou fichier) : le contenu ENTIER, saut de ligne final
     * compris — règle de l'étalon gtk3sermo. Jusqu'à la 2.7.0, <text> ignorait
     * <input>. Le cœur appelle cette fonction juste après la création. */
    gchar *text = widget_input_text(var->Attributes);
    if (text) {
        ((TexteOut *)var->Widget)->poser(text);
        g_free(text);
    }
    ((Fl_Widget *)var->Widget)->redraw();
}

void widget_text_fileselect(variable *var, const char *n, const char *v) {}
void widget_text_removeselected(variable *var) {}
void widget_text_save(variable *var) {}
