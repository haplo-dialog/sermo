/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mavlink-commun.h — ce que partagent sermo-source-mavlink (lecture) et
 * sermo-cible-mavlink (écriture) : l'adresse du lien et son ouverture.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Adresses :
 *   udp://[hôte]:port     écoute UDP (hôte vide : 127.0.0.1)
 *   tcp://hôte:port       connexion TCP (simulateur SITL : 5760)
 *   serie:///dev/ttyUSB0  liaison série (radio USB), débit par la clé vitesse
 */
#ifndef SERMO_MAVLINK_COMMUN_H
#define SERMO_MAVLINK_COMMUN_H

#include <stddef.h>
#include <termios.h>

typedef enum { LIEN_UDP, LIEN_TCP, LIEN_SERIE } transport_t;

typedef struct {
	transport_t transport;
	char hote[256], port[16], chemin[1024];
	long vitesse;
	const char *base;
} lien_t;

long mavlink_maintenant_ms(void);

/* Débit termios de @v, ou 0 s'il n'est pas un débit série connu. */
speed_t mavlink_debit(long v);

/* Remplit @l depuis @base (avant le « ? »). Rend 0, ou -1 avec un message. */
int lien_analyser(const char *programme, const char *base, lien_t *l);

/* Ouvre le lien, la connexion TCP bornée par @fin (mavlink_maintenant_ms).
 * @ecriture = 0 : la liaison série est ouverte en LECTURE SEULE (le lecteur
 * n'écrit jamais rien). Rend un descripteur non bloquant, ou -1 (message dit). */
int lien_ouvrir(const char *programme, const lien_t *l, long fin, int ecriture);

#endif
