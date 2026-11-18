/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * ajouter_sauver.h — « append: » et les rangées de « save: » pour les ports
 * dont les widgets sont des WidgetNode (dialog_state.h) : sdl3, lvgl, ncurses,
 * html. Jusqu'à la 3.3.0, append: n'y faisait rien et save: n'écrivait rien.
 *
 * Deux fonctions du contrat (sermo-contract.h), que le cœur appelle :
 *   sermo_be_widget_append — comme l'étalon gtk3sermo : <entry>, le texte est
 *     remplacé ; <edit>, il est ajouté à la fin ; <list>, une rangée est
 *     ajoutée EN TÊTE et choisie ; <tree> et <table>, une rangée À LA FIN ;
 *   sermo_be_widget_rows — toutes les rangées d'une <list>, d'un
 *     <comboboxtext>, d'un <comboboxentry>, d'un <tree> ou d'une <table>,
 *     une par ligne (colonnes séparées par « | » ; le cœur les ramène au
 *     nombre de colonnes du <label>).
 *
 * À inclure UNE fois par port (widgets.c). Le même fichier est repris tel
 * quel dans chacun des quatre ports.
 */
#ifndef SERMO_AJOUTER_SAUVER_H
#define SERMO_AJOUTER_SAUVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "automaton.h"
#include "dialog_state.h"

int   sermo_be_widget_append(GtkWidget *w, int type, const char *valeur);
void  sermo_table_ajouter(WidgetNode *node, const char *line);   /* widget_table.c */
char *sermo_be_widget_rows(GtkWidget *w, int type);

int sermo_be_widget_append(GtkWidget *w, int type, const char *valeur)
{
    WidgetNode *n = (WidgetNode *)w;

    if (!n || !valeur) return 0;
    switch (type) {
    case WIDGET_ENTRY:
        snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", valeur);
        return 1;
    case WIDGET_EDIT: {
        size_t avant = n->state.text.content ? strlen(n->state.text.content) : 0;
        size_t ajout = strlen(valeur);
        char *t = (char *)realloc(n->state.text.content, avant + ajout + 1);
        if (!t) return 0;
        memcpy(t + avant, valeur, ajout + 1);
        n->state.text.content = t;
        n->state.text.len = (int)(avant + ajout);
        /* cap = taille RÉELLE du tampon (voir widget_edit_refresh) */
        n->state.text.cap = n->state.text.len + 1;
        return 1;
    }
    case WIDGET_LIST: {
        int nb = n->state.list.item_count;
        char *copie = strdup(valeur);
        char **items;
        if (!copie) return 0;
        items = (char **)realloc(n->state.list.items, ((size_t)nb + 1) * sizeof(char *));
        if (!items) { free(copie); return 0; }
        memmove(items + 1, items, (size_t)nb * sizeof(char *));
        items[0] = copie;
        n->state.list.items = items;
        n->state.list.item_count = nb + 1;
        n->state.list.selected_index = 0;
        return 1;
    }
    case WIDGET_TREE: {
        /* un <tree> garde ses rangées comme une liste (state.list) */
        int nb = n->state.list.item_count;
        char *copie = strdup(valeur);
        char **items;
        if (!copie) return 0;
        items = (char **)realloc(n->state.list.items, ((size_t)nb + 1) * sizeof(char *));
        if (!items) { free(copie); return 0; }
        items[nb] = copie;
        n->state.list.items = items;
        n->state.list.item_count = nb + 1;
        return 1;
    }
    case WIDGET_TABLE:
        sermo_table_ajouter(n, valeur);         /* une rangée À LA FIN */
        return 1;
    default:
        return 0;
    }
}

/* Les rangées d'une <table> : ses cellules, « | » entre les colonnes. */
static char *rangees_de_table(const WidgetNode *n)
{
    int cols = n->state.table.cols, rows = n->state.table.rows;
    size_t taille = 1, pos = 0;
    char *r;

    if (cols < 1 || !n->state.table.cells) rows = 0;
    for (int i = 0; i < rows * cols; i++)
        taille += (n->state.table.cells[i] ? strlen(n->state.table.cells[i]) : 0) + 1;
    r = (char *)malloc(taille);
    if (!r) return NULL;
    for (int l = 0; l < rows; l++) {
        if (l) r[pos++] = '\n';
        for (int c = 0; c < cols; c++) {
            const char *t = n->state.table.cells[l * cols + c];
            size_t lg = t ? strlen(t) : 0;
            if (c) r[pos++] = '|';
            if (lg) memcpy(r + pos, t, lg);
            pos += lg;
        }
    }
    r[pos] = '\0';
    return r;
}

char *sermo_be_widget_rows(GtkWidget *w, int type)
{
    WidgetNode *n = (WidgetNode *)w;
    size_t taille = 1, pos = 0;
    char *r;

    if (!n) return NULL;
    if (type == WIDGET_TABLE)
        return rangees_de_table(n);
    if (type != WIDGET_LIST && type != WIDGET_COMBOBOXTEXT && type != WIDGET_COMBOBOXENTRY
        && type != WIDGET_TREE)
        return NULL;
    for (int i = 0; i < n->state.list.item_count; i++)
        taille += strlen(n->state.list.items[i]) + 1;
    r = (char *)malloc(taille);
    if (!r) return NULL;
    for (int i = 0; i < n->state.list.item_count; i++) {
        size_t l = strlen(n->state.list.items[i]);
        if (i) r[pos++] = '\n';
        memcpy(r + pos, n->state.list.items[i], l);
        pos += l;
    }
    r[pos] = '\0';
    return r;
}

#endif /* SERMO_AJOUTER_SAUVER_H */
