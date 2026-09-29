/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_comboboxtext.cpp — ComboBox texte FLTK (Fl_Choice)
 * sermo — haplo-dialog — GPL-2.0-or-later
 *
 * Sert aussi <comboboxentry> (le cœur route les deux types ici).
 * Les éléments se chargent dans refresh, comme chez l'étalon gtk3sermo : le
 * cœur l'appelle juste après la création, <input> passe AVANT les <item>, et
 * <default> doit voir les éléments venus de <input>. */
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
#include "widget_comboboxtext.h"
#include <FL/Fl_Choice.H>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace {

/* L'étalon distingue le PREMIER refresh (<default> appliqué) des suivants
 * (éléments vidés puis rechargés) ; g_object_set_data est un no-op dans
 * fltk-compat.h, d'où ce drapeau. Type créé ET relu dans ce seul fichier. */
class SermoComboText : public Fl_Choice {
public:
    SermoComboText(int x, int y, int w, int h)
        : Fl_Choice(x, y, w, h), initialise(false) {}
    bool initialise;
};

/* Libellé d'élément de menu : FLTK 1.4.4 le recopie SANS borne dans un
 * tampon de pile de 1024 octets (Fl_Menu_Item::insert, lu au désassemblage ;
 * mesuré : « stack smashing detected » avec un libellé de 1100 octets). */
const size_t CHOICE_LABEL_MAX = 1023;

/* Ajoute UN élément dont le libellé est le texte tel quel. Mesuré : add(s)
 * découpe sur « | », et add(s, 0, …) lit « / » comme un sous-menu, « \ »
 * comme un échappement et un « _ » initial comme un séparateur. Une ligne de
 * commande ou de fichier est du texte : on échappe, et on borne. */
void choice_add_literal(Fl_Choice *ch, const char *text)
{
    size_t len = strlen(text);

    if (len > CHOICE_LABEL_MAX) {
        len = CHOICE_LABEL_MAX;
        /* ne pas couper un caractère UTF-8 en deux */
        while (len > 0 && ((unsigned char)text[len] & 0xC0) == 0x80)
            len--;
    }

    GString *s = g_string_sized_new(len + 16);
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '/' || text[i] == '\\' || (text[i] == '_' && i == 0))
            g_string_append_c(s, '\\');
        g_string_append_c(s, text[i]);
    }
    ch->add(s->str, 0, NULL, NULL, 0);
    g_string_free(s, TRUE);
}

/* <default> : l'étalon compare le TEXTE de l'élément. find_item() suit les
 * chemins de menu (« a/b ») : gardé en repli pour les <item> qui en créent. */
void choice_select_text(Fl_Choice *ch, const char *text)
{
    const Fl_Menu_Item *m = ch->menu();

    for (int i = 0; m != NULL && i < ch->size() - 1; i++) {
        if (m[i].label() != NULL && strcmp(m[i].label(), text) == 0) {
            ch->value(i);
            return;
        }
    }
    if (const Fl_Menu_Item *found = ch->find_item(text))
        ch->value(found);
}

} /* namespace */

GtkWidget *widget_comboboxtext_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void)Attr; (void)attr; (void)Type;
    /* Vide : <input>, <item> et <default> sont chargés par refresh. */
    return (GtkWidget *)new SermoComboText(0, 0, 200, 30);
}
gchar *widget_comboboxtext_envvar_construct(GtkWidget *w)
{
    Fl_Choice *c = (Fl_Choice *)w;
    const Fl_Menu_Item *it = c->mvalue();
    return g_strdup(it && it->label() ? it->label() : "");
}
gchar *widget_comboboxtext_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_comboboxtext_envvar_construct(v->Widget) : NULL; }
void widget_comboboxtext_clear(variable *v)
{ if (v && v->Widget) ((Fl_Choice *)v->Widget)->value(0); }
void widget_comboboxtext_refresh(variable *v)
{
    if (!v || !v->Widget) return;
    SermoComboText *ch = dynamic_cast<SermoComboText *>(v->Widget);
    if (!ch) { v->Widget->redraw(); return; }

    /* Déjà initialisé : l'étalon vide avant de recharger (sinon doublons). */
    if (ch->initialise)
        ch->clear();

    /* <input> (commande ou fichier, décodé) : une ligne = un élément, lignes
     * vides comprises — l'étalon gtk3sermo les garde (mesuré : « \nun\n »
     * sélectionne l'élément vide). */
    gchar **lignes = widget_input_lines(v->Attributes);
    for (gchar **l = lignes; l && *l; l++)
        choice_add_literal(ch, *l);
    g_strfreev(lignes);

    GList *el = NULL;
    gchar *item = attributeset_get_first(&el, v->Attributes, ATTR_ITEM);
    while (item) { if (*item) ch->add(item); item = attributeset_get_next(&el, v->Attributes, ATTR_ITEM); }

    /* Parité étalon : un comboboxtext sélectionne son élément 0 ; un
     * comboboxentry (éditable, nu) ne sélectionne RIEN. */
    if (v->Type == WIDGET_COMBOBOXTEXT && ch->size() > 1)
        ch->value(0);

    /* <default> : au premier refresh seulement, comme l'étalon. */
    if (!ch->initialise) {
        el = NULL;
        gchar *def = attributeset_get_first(&el, v->Attributes, ATTR_DEFAULT);
        if (def) choice_select_text(ch, def);
        ch->initialise = true;
    }
    ch->redraw();
}
void widget_comboboxtext_fileselect(variable *v, const char*, const char*) {}
void widget_comboboxtext_removeselected(variable *v) {}
void widget_comboboxtext_save(variable *v) {}
