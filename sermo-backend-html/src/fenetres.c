/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * fenetres.c — voir fenetres.h.
 * Le même fichier est repris tel quel dans chaque port qui en a besoin.
 */
#include <stdlib.h>

#include "fenetres.h"

static WidgetNode *g_fenetres[FENETRES_MAX];
static int         g_nb;
static int         g_lancees;

void (*fenetre_au_retrait)(WidgetNode *racine) = NULL;

int         fenetres_nb(void)    { return g_nb; }
WidgetNode *fenetre(int i)       { return (i >= 0 && i < g_nb) ? g_fenetres[i] : NULL; }
WidgetNode *fenetre_dessus(void) { return g_nb > 0 ? g_fenetres[g_nb - 1] : NULL; }

WidgetNode *fenetre_racine(void *noeud)
{
    WidgetNode *n = (WidgetNode *)noeud;
    while (n && n->parent) n = n->parent;
    return n;
}

static int rang_de(const WidgetNode *racine)
{
    for (int i = 0; i < g_nb; i++)
        if (g_fenetres[i] == racine) return i;
    return -1;
}

int fenetre_ouverte(void *noeud)
{
    return rang_de(fenetre_racine(noeud)) >= 0;
}

static void retirer(int i)
{
    for (; i + 1 < g_nb; i++) g_fenetres[i] = g_fenetres[i + 1];
    g_nb--;
}

int fenetre_empiler(WidgetNode *racine)
{
    if (!racine || rang_de(racine) >= 0) return 1;
    if (g_nb >= FENETRES_MAX || g_lancees >= FENETRES_MAX) return 0;
    g_lancees++;
    g_fenetres[g_nb++] = racine;
    return 1;
}

/* L'arbre n'est pas libéré : l'action peut venir d'un de ses widgets. */
int sermo_be_window_close(void *w)
{
    WidgetNode *r = fenetre_racine(w);
    int i = rang_de(r);
    if (i < 0) return 0;
    retirer(i);
    if (fenetre_au_retrait) fenetre_au_retrait(r);
    return 1;
}

void sermo_be_window_present(void *w)
{
    WidgetNode *r = fenetre_racine(w);
    int i = rang_de(r);
    if (i < 0) return;
    retirer(i);
    g_fenetres[g_nb++] = r;
}

/* ─── Minuteries sans horloge ───────────────────────────────────────────────
 * state.scale.value de la minuterie : 0 = pas encore armée, > 0 = échéance
 * fictive (secondes), < 0 = déjà jouée. */
static double g_heure;   /* heure fictive : l'échéance de la dernière jouée */

static double intervalle_s(const WidgetNode *n)
{
    int ms = (n->label && *n->label) ? atoi(n->label) : 1000;
    return (ms > 0 ? ms : 1000) / 1000.0;
}

/* La prochaine à jouer : la plus petite échéance ; à égalité, la première
 * dans l'ordre des fenêtres puis du document. */
static void chercher(WidgetNode *n, WidgetNode **choix)
{
    if (!n) return;
    if (n->type == WT_TIMER && n->sensitive && n->state.scale.value >= 0) {
        if (n->state.scale.value == 0)
            n->state.scale.value = g_heure + intervalle_s(n);
        if (!*choix || n->state.scale.value < (*choix)->state.scale.value)
            *choix = n;
    }
    for (int i = 0; i < n->child_count; i++) chercher(n->children[i], choix);
}

void minuteries_une_fois(void (*tic)(WidgetNode *minuterie))
{
    for (;;) {
        WidgetNode *m = NULL;
        for (int i = 0; i < g_nb; i++) chercher(g_fenetres[i], &m);
        if (!m) return;
        g_heure = m->state.scale.value;
        m->state.scale.value = -1;
        tic(m);   /* exit:… sort ici ; launch: rappelle cette fonction et ne revient pas */
    }
}
