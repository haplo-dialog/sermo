/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_focus.h — quel widget reçoit le focus à l'ouverture d'une fenêtre.
 *
 * gtk3sermo (l'étalon) choisit la première ligne d'une <list>, d'un <tree> ou
 * d'un <table> seulement quand ce widget reçoit le focus à l'ouverture : c'est
 * le cas s'il est le PREMIER widget focalisable du document, visible et actif
 * (mesuré le 2026-10-11 : une entrée, un bouton, une case, un calendrier, une
 * glissière ou une liste déroulante placés avant lui le privent de sélection ;
 * un texte, un widget invisible ou inactif, non ; un <notebook> est lui-même
 * focalisable, donc une liste dans un onglet n'est jamais choisie d'office).
 * Le lexer note chaque balise ouvrante dans l'ordre du document ; chaque port
 * demande, à la création d'une liste, si c'est elle qui prend le focus.
 */
#ifndef SERMO_FOCUS_H
#define SERMO_FOCUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Appelées par le lexer. */
void sermo_focus_ouvre(const char *texte_balise);        /* « <entry ... » */
void sermo_focus_attribut(const char *nom);              /* nom d'attribut lu */
void sermo_focus_valeur(const char *valeur);             /* sa valeur */
void sermo_focus_contenu(const char *texte);             /* texte entre balises */

/* Appelée par un port à la création d'une <list>, d'un <tree> ou d'un <table>
 * (balise = "list", "tree" ou "table"), dans l'ordre de création, qui suit
 * l'ordre du document pour une même balise : vrai si ce widget est le premier
 * focalisable de sa fenêtre, donc si sa première ligne doit être choisie. */
int sermo_focus_initial(const char *balise);

#ifdef __cplusplus
}
#endif

#endif /* SERMO_FOCUS_H */
