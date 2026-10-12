/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_vbox.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Vrai/faux d'un attribut space-* comme l'étalon ; -1 s'il est absent. */
static int valeur_space(tag_attr *attr, const char *nom, int defaut)
{
    const char *v = attr ? get_tag_attribute(attr, nom) : NULL;
    if (!v) return defaut;
    return (strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 ||
            atoi(v) == 1) ? 1 : 0;
}

/* Rangement des enfants d'une boîte, calculé à sa création comme gtk3
 * (sermo-backend-gtk3/src/widget_vbox.c et widget_hbox.c) :
 *   - valeur courante = celle de la boîte, sinon l'option du projet ;
 *   - parcours : colonne dans l'ordre, RANGÉE à l'envers (pack_end) ;
 *   - un enfant qui dit space-expand / space-fill REMPLACE la valeur
 *     courante, pour lui et pour ceux parcourus après (jamais remise à zéro) ;
 *   - valeur non dite : défaut du type empilé (colonne : edit, frame,
 *     fenêtre défilante, paned ; rangée : edit, frame, fenêtre défilante,
 *     entry).
 * L'attribut propre de l'enfant est déjà posé (widgets.c, liste de
 * visibilité, appelée juste après la création de chaque enfant). */
void sdl3_box_pack(WidgetNode *box, stackelement *s, tag_attr *attr, int rangee)
{
    int expand = valeur_space(attr, "space-expand", project_space_expand);
    int fill   = valeur_space(attr, "space-fill", project_space_fill);
    const char *h = attr ? get_tag_attribute(attr, "homogeneous") : NULL;
    box->homogeneous = h && (strcasecmp(h, "true") == 0 ||
                             strcasecmp(h, "yes") == 0 || atoi(h) == 1);
    for (int k = 0; k < s->nwidgets; ++k) {
        int i = rangee ? s->nwidgets - 1 - k : k;
        WidgetNode *ch = (WidgetNode *)s->widgets[i];
        if (!ch) continue;
        if (ch->expand) expand = (ch->expand == 1);
        if (ch->fill)   fill   = (ch->fill == 1);
        int t = s->widgettypes[i];
        int defaut = (t == WIDGET_EDIT || t == WIDGET_FRAME ||
                      t == WIDGET_SCROLLEDW ||
                      (rangee ? t == WIDGET_ENTRY : t == WIDGET_PANED));
        ch->pk_expand = (expand != -1) ? expand : defaut;
        ch->pk_fill   = (fill   != -1) ? fill   : defaut;
    }
}

GtkWidget *widget_vbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_VBOX, NULL, "");
    /* Le coeur fusionne les enfants en UN element de pile (instruction
     * SUM) : UN SEUL pop, dans l'ordre du document. L'ancienne boucle
     * « pop jusqu'a une sentinelle » provoquait un stack underflow sur
     * TOUT dialogue (la sentinelle n'existe pas). */
    stackelement s = pop();
    for (int i = 0; i < s.nwidgets; ++i)
        if (s.widgets[i]) widget_node_add_child(node, (WidgetNode *)s.widgets[i]);
    sdl3_box_pack(node, &s, attr, 0);
    return (GtkWidget *)node;
}

gchar *widget_vbox_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_vbox_envvar_all_construct(variable *var) { return NULL; }
void   widget_vbox_clear(variable *var) {}
void   widget_vbox_refresh(variable *var) {}
void   widget_vbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_vbox_removeselected(variable *var) {}
void   widget_vbox_save(variable *var) {}
