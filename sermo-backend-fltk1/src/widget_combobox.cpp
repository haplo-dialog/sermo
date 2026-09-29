/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_combobox.cpp — Liste déroulante FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <combobox> → Fl_Choice (liste déroulante non éditable)
 *
 * Alimentation des items :
 *   - <item>texte</item>  dans le XML → via ATTR_ITEM dans l'AttributeSet
 *   - <input> (commande ou fichier) : NON implémenté, comme chez l'étalon
 *     gtk3sermo — avertissement, rien n'est exécuté ni lu
 *
 * Export : texte de l'item sélectionné (ou "" si vide)
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
#include "widget_combobox.h"
#include "safe_exec.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_combobox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int    w = 200, h = 26;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    Fl_Choice *ch = new Fl_Choice(0, 0, w, h, nullptr);

    if (Attr) {
        /* Items statiques <item> */
        element = NULL;
        gchar *item = attributeset_get_first(&element, Attr, ATTR_ITEM);
        while (item) {
            if (*item) ch->add(item);
            item = attributeset_get_next(&element, Attr, ATTR_ITEM);
        }

        /* Sélection initiale via <default> */
        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) {
            /* Chercher l'item correspondant */
            int found = -1;
            for (int i = 0; i < ch->size() - 1; i++) {
                const Fl_Menu_Item *mi = ch->menu() + i;
                if (mi->label() && strcmp(mi->label(), def) == 0) {
                    found = i; break;
                }
            }
            if (found >= 0) ch->value(found);
        }
    }

    /* Parité étalon : un combobox nu ne sélectionne RIEN par défaut
     * (CO="") — l'auto-sélection du premier item était un écart mesuré. */

    return (GtkWidget *)ch;
}

gchar *widget_combobox_envvar_construct(GtkWidget *widget)
{
    Fl_Choice *ch = (Fl_Choice *)widget;
    if (!ch) return g_strdup("");
    const Fl_Menu_Item *mi = ch->mvalue();
    if (!mi || !mi->label()) return g_strdup("");
    return g_strdup(mi->label());
}

gchar *widget_combobox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_combobox_envvar_construct(var->Widget);
}

void widget_combobox_clear(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Choice *ch = (Fl_Choice *)var->Widget;
    ch->clear();
}

void widget_combobox_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    Fl_Choice *ch = (Fl_Choice *)var->Widget;

    /* <input> n'est PAS implémenté pour <combobox> chez l'étalon gtk3sermo :
     * un avertissement par directive, ni commande exécutée ni fichier lu.
     * (Le port lançait « Command:… » brut, deux fois : création + refresh.) */
    GList *element = NULL;
    for (gchar *act = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
         act != NULL;
         act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT)) {
        if (strncasecmp(act, "Command:", 8) == 0)
            g_warning("%s(): <input> not implemented for this widget.", __func__);
        else if (strncasecmp(act, "file:", 5) == 0 && act[5] != '\0')
            g_warning("%s(): <input file> not implemented for this widget.", __func__);
    }
    ch->redraw();
}

void widget_combobox_fileselect(variable *var, const char *n, const char *v) {}
void widget_combobox_removeselected(variable *var) {}
void widget_combobox_save(variable *var) {}
