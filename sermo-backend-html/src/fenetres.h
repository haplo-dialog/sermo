/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * fenetres.h — les fenêtres ouvertes d'un port à arbre de WidgetNode
 * (launch: / closewindow:, manuel §9.2).
 *
 * Le cœur lance une fenêtre en rappelant la boucle du port depuis une action
 * (gtk_main imbriqué, qui ne rend jamais la main) et la ferme par
 * sermo_be_window_close(). Jusqu'à la 3.3.0 ces ports ne tenaient qu'UNE
 * racine : la seconde boucle ne servait que la fenêtre lancée, la première
 * était perdue, et le dialogue sortait en EXIT="abort" (html, ncurses sans
 * terminal) ou se figeait (lvgl).
 *
 * Ici : la pile des racines ouvertes — la dernière lancée est « dessus » — et
 * le jeu des minuteries quand il n'y a pas d'horloge (sans écran).
 *
 * Le même fichier est repris tel quel dans chaque port qui en a besoin.
 */
#ifndef SERMO_FENETRES_H
#define SERMO_FENETRES_H

#include "dialog_state.h"   /* WidgetNode */

#define FENETRES_MAX 64

int         fenetres_nb(void);
WidgetNode *fenetre(int i);
WidgetNode *fenetre_dessus(void);          /* NULL : plus aucune fenêtre */
WidgetNode *fenetre_racine(void *noeud);   /* la racine de l'arbre d'un nœud */
int         fenetre_ouverte(void *noeud);

/* Ajoute une racine à la pile (déjà dedans : rien). Rend 0 si le dialogue a
 * lancé plus de FENETRES_MAX fenêtres — un launch: qui se relance sans fin. */
int         fenetre_empiler(WidgetNode *racine);

/* Crochets du port, facultatifs : appelés quand une fenêtre quitte la pile ou
 * repasse dessus (lvgl y range son écran). */
extern void (*fenetre_au_retrait)(WidgetNode *racine);

/* Contrat (facultatif) : closewindow:, et launch: d'une fenêtre déjà ouverte. */
int  sermo_be_window_close(void *w);
void sermo_be_window_present(void *w);

/* Sans écran, pas d'horloge : chaque minuterie de chaque fenêtre ouverte joue
 * UNE fois, dans l'ordre où elles partiraient à l'écran. Une horloge fictive
 * avance d'échéance en échéance ; une fenêtre lancée en cours de route arme
 * les siennes à l'heure fictive de son lancement, une fenêtre fermée emporte
 * les siennes. Rappelée depuis une action (launch:), la fonction reprend la
 * file là où elle en est. Revient quand plus rien n'est à jouer. */
void minuteries_une_fois(void (*tic)(WidgetNode *minuterie));

#endif
