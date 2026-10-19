/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * media-commun.h — ce que partagent sermo-cible-media (commandes d'un lecteur
 * son/vidéo) et sermo-source-media (son état).
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * UN LECTEUR = un mpv, désigné par un NOM (media://NOM?…), avec son dossier
 * privé $XDG_RUNTIME_DIR/sermo-media/NOM/ (700) :
 *   ipc        socket de commande de mpv (600, créé par mpv)
 *   gardien    PID du gardien (il tue mpv quand sermo disparaît)
 *   mpv        PID de mpv (ou de bwrap qui l'enveloppe)
 *   fin        écrit par le gardien quand mpv s'arrête : « fin » (lecture
 *              terminée), « arret » (demandé), « erreur » (mpv en échec)
 *
 * SÉCURITÉ. La commande IPC « run » de mpv exécute du shell (mesuré, J0) :
 * le socket et son dossier doivent n'appartenir qu'à l'utilisateur, et seules
 * les quelques requêtes JSON écrites ici, avec des nombres formatés par nous,
 * y partent jamais une chaîne venue de l'utilisateur.
 */
#ifndef SERMO_MEDIA_COMMUN_H
#define SERMO_MEDIA_COMMUN_H

#include <stddef.h>
#include <sys/types.h>

#define MEDIA_LECTEURS_MAX 4
#define MEDIA_NOM_MAX 32

/* « media://NOM » → NOM dans @nom (≤ 32 caractères [A-Za-z0-9_-]). 0 ou -1 + message. */
int media_nom(const char *programme, const char *base, char *nom, size_t taille);

/* $XDG_RUNTIME_DIR/sermo-media (créé en 700 au besoin, propriétaire vérifié). */
int media_racine(const char *programme, char *chemin, size_t taille, int creer);

/* Dossier du lecteur NOM ; @creer : le créer (700). Propriétaire et mode vérifiés. */
int media_dossier(const char *programme, const char *nom, char *chemin, size_t taille, int creer);

/* PID lu dans <dossier>/<fichier>, s'il est vivant ; 0 sinon. */
pid_t media_pid_vivant(const char *dossier, const char *fichier);

/* Envoie @requete (une ligne JSON écrite par nous) au socket du lecteur et range
 * dans @reponse la ligne de réponse (les lignes d'évènement sont sautées).
 * Vérifie avant de se connecter que le socket est à l'utilisateur, en 600.
 * 0, ou -1 (pas de lecteur, refus, délai de 2 s). */
int media_ipc(const char *dossier, const char *requete, char *reponse, size_t taille);

/* Valeur « $NOM » lue dans l'environnement (première ligne), ou @brute telle quelle. */
const char *media_valeur(const char *programme, const char *brute);

#endif
