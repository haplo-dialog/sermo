/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_filechooser.c — Sélecteur de fichier SDL3 (dialogue NATIF)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <filechooser> : l'étalon gtk3 ouvre un GtkFileChooser. Le port SDL3 n'a pas
 * besoin d'en réécrire un — depuis SDL 3.2, la bibliothèque expose le VRAI
 * sélecteur du système : SDL_ShowOpenFileDialog / SDL_ShowSaveFileDialog /
 * SDL_ShowOpenFolderDialog (portail XDG sous Wayland, natif ailleurs). Le
 * bouton « Parcourir… » du renderer l'ouvre, le rappel écrit le chemin retenu
 * dans state.entry.buf. L'ancienne implémentation « mini » se contentait de
 * reprendre <default> : le banc passait, mais l'utilisateur n'avait aucun
 * moyen de CHOISIR. Aucune dépendance ajoutée : c'est du SDL3 nu.
 *
 * Attribut de balise « action » : open (défaut) | save | select-folder —
 * select-folder ne propose que des répertoires.
 *
 * Export : le chemin retenu, comme l'étalon gtk3.
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
#include "widget_filechooser.h"
#include <stdlib.h>

GtkWidget *widget_filechooser_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;
    WidgetNode *node = widget_node_new(WT_FILECHOOSER, NULL, "");

    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def)
            snprintf(node->state.entry.buf, sizeof(node->state.entry.buf), "%s", def);
        el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
    }
    /* Mode : « select-folder » ne retient que des répertoires. Rangé dans
     * tooltip, seul champ libre du nœud (le renderer le relit tel quel). */
    if (attr) {
        const char *a = get_tag_attribute(attr, "action");
        if (a && *a) node->tooltip = strdup(a);
    }
    return (GtkWidget *) node;
}

gchar *widget_filechooser_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *) widget;
    if (!n) return g_strdup("");
    return g_strdup(n->state.entry.buf);
}
gchar *widget_filechooser_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_filechooser_envvar_construct(var->Widget);
}
void widget_filechooser_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *) var->Widget)->state.entry.buf[0] = '\0';
}
void widget_filechooser_refresh(variable *var) { (void) var; }
void widget_filechooser_fileselect(variable *var, const char *name, const char *value)
{
    /* Le cœur pousse un chemin choisi ailleurs (action fileselect). */
    (void) name;
    if (!var || !var->Widget || !value) return;
    WidgetNode *n = (WidgetNode *) var->Widget;
    snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", value);
}
void widget_filechooser_removeselected(variable *var) { (void) var; }
void widget_filechooser_save(variable *var)           { (void) var; }
