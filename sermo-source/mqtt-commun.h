/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mqtt-commun.h — ce que partagent sermo-source-mqtt (lecture) et
 * sermo-cible-mqtt (écriture) : adresse mqtt[s]://, certificats, identifiants,
 * connexion bornée par un délai, classement des échecs du chiffrement.
 * Une seule copie, pour que lecture et écriture ne divergent jamais.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef SERMO_MQTT_COMMUN_H
#define SERMO_MQTT_COMMUN_H

#include <mosquitto.h>
#include <stddef.h>

#include "commun.h"

/* Ce que l'adresse demande, vérifié : serveur, chiffrement, fichiers. */
typedef struct {
	int tls;                       /* mqtts:// */
	char hotes[4096];              /* copie de la base, découpée en place */
	char *hote;
	int port;
	const char *confiance, *certificat, *cle;
	char *utilisateur, *mot;       /* lus dans le fichier d'identifiants */
} mqtt_reglage_t;

/* Découpe une base mqtt[s]://hôte[:port] : chiffrement, hôte (copié dans
 * @hote, @n octets), port (1883 / 8883 par défaut). Rend 0 ou -1. */
int mqtt_base(const char *base, int *tls, char *hote, size_t n, int *port);

/* Vérifie la base (mqtt:// ou mqtts://, hôte, port), les clés confiance,
 * certificat, cle (mqtts:// seulement, confiance obligatoire, clé en 600) et
 * identifiants (fichier en 600). Rend 0, ou -1 avec la raison sur la sortie
 * d'erreur. */
int mqtt_preparer(const char *programme, const adresse_t *a, mqtt_reglage_t *r);
void mqtt_reglage_liberer(mqtt_reglage_t *r);

/* Pose le chiffrement (vérification du certificat ET du nom, jamais
 * « insecure ») et les identifiants sur @m. Rend 0 ou -1 (message dit). */
int mqtt_appliquer(const char *programme, struct mosquitto *m, const mqtt_reglage_t *r);

/* Connexion BLOQUANTE (chaque adresse du nom est essayée, poignée de main
 * chiffrée comprise), interrompue par une minuterie au bout de @delai ms.
 * Rend le code de libmosquitto ; *@erreur reçoit errno ; *@expire vaut 1 si le
 * délai est passé. */
int mqtt_connecter(struct mosquitto *m, const mqtt_reglage_t *r, long delai, int *erreur, int *expire);

/* Vrai si @rc (et @erreur) signale un échec du chiffrement, pas une panne. */
int mqtt_echec_tls(int tls, int rc, int erreur);
/* Dit le refus du chiffrement sur la sortie d'erreur. */
void mqtt_dire_refus_tls(const char *programme, const char *base);
/* Raison lisible d'un échec de connexion. */
const char *mqtt_raison(int rc, int erreur);

long mqtt_maintenant_ms(void);

#endif
