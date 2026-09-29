/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_input.h — une limite de taille pour ce que lit un <input>.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * POURQUOI. Jusqu'à la 2.7.2, un <input> branché sur une source sans fin — une
 * commande comme `yes`, un fichier comme /dev/zero — lisait jusqu'à épuiser la
 * mémoire. Mesuré le 2026-09-17 sur les binaires des paquets : gtk3sermo passe
 * 700 Mo en 7 secondes ; ncurses finit sur une erreur de segmentation au lieu
 * d'un arrêt propre. La frontière de confiance reste l'auteur du script, mais
 * une donnée venue d'ailleurs ne doit pas pouvoir faire tomber le dialogue.
 *
 * CONTRAT.
 *  - Chaque <input> lit au plus SERMO_INPUT_MAX octets : 16 Mio par défaut ;
 *    la variable d'environnement SERMO_INPUT_MAX (en octets) la change, 0 la
 *    retire. Une valeur illisible est signalée une fois et la limite par défaut
 *    s'applique.
 *  - Au-delà, la lecture s'arrête comme sur une fin de fichier, et un
 *    avertissement part UNE fois sur la sortie d'erreur — jamais sur la sortie,
 *    qu'un `eval` lit. Une source qui fait exactement la limite n'est pas
 *    signalée : il faut un octet de plus pour parler de troncature.
 *  - Une commande coupée ainsi reçoit SIGPIPE à sa prochaine écriture, quand le
 *    flux est fermé : `yes` s'arrête de lui-même.
 *  - Le flux rendu se ferme par fclose(), comme avant. Il n'a pas de descripteur
 *    propre : fileno() y rend -1.
 *  - EXCEPTION : la barre de progression. Elle lit une ligne à la fois dans un
 *    tampon fixe et n'accumule rien ; elle doit pouvoir suivre une commande
 *    longue — une copie qui affiche chaque nom de fichier dépasse vite 16 Mio.
 *    La couper tuerait la commande en route. Elle reprend donc le flux réel par
 *    sermo_input_sans_limite().
 */
#ifndef SERMO_INPUT_H
#define SERMO_INPUT_H

#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* La limite par défaut : 16 Mio. */
#define SERMO_INPUT_MAX_DEFAUT ((size_t) 16 * 1024 * 1024)

/* La limite en vigueur, en octets ; 0 = aucune. */
size_t sermo_input_max(void);

/* Plafonne un flux de lecture neuf — ouvert par fopen() ou fdopen(), pas encore
 * lu — et en prend la charge : le flux rendu le ferme à son tour. @source nomme
 * l'<input> dans l'avertissement. Sans limite (0), ou si le plafonnement est
 * impossible, rend @flux tel quel. NULL rend NULL. */
FILE *sermo_input_wrap(FILE *flux, const char *source);

/* fopen(@chemin, "r"), plafonné : la lecture d'un <input file>. */
FILE *sermo_fopen_input(const char *chemin);

/* Retire la limite d'un flux rendu par sermo_input_wrap() ou safe_popen(), et
 * rend le flux réel, que l'appelant fermera par fclose(). Pour un lecteur qui
 * n'accumule rien : la barre de progression. À appeler AVANT toute lecture —
 * ce que le flux plafonné aurait déjà mis en tampon serait perdu. Un flux
 * ordinaire est rendu tel quel ; NULL rend NULL. */
FILE *sermo_input_sans_limite(FILE *flux);

#ifdef __cplusplus
}
#endif

#endif /* SERMO_INPUT_H */
