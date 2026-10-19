/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo-source-mqtt — programme d'aide de <input mqtt> : lit le message d'un
 * sujet MQTT (domotique) et l'écrit sur la sortie standard.
 * Lecture SEULE : le programme s'abonne et lit ; il n'appelle jamais
 * mosquitto_publish(). Publier vers un sujet sera le rôle d'un programme à part,
 * dans son propre paquet (sermo-cible-mqtt).
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Appelé par sermo (voir libsermocore/include/sermo_source.h) avec l'adresse
 * encodée en un argument :
 *   mqtt://hôte[:port]?sujet=maison/salon/temperature[&clé=valeur…]
 *   mqtts://hôte[:port]?sujet=…&confiance=/chemin/ac.pem[&…]   (chiffré)
 *   (ports 1883 et 8883 par défaut ; hôte : nom, IPv4 ou [IPv6])
 * Clés :
 *   sujet         obligatoire : un sujet précis. Les jokers « + » et « # » sont
 *                 refusés : un widget montre UNE valeur.
 *   delai         délai total en ms (connexion + attente du message ; défaut
 *                 1500, de 10 à 60000). Le message retenu (« retained ») arrive
 *                 tout de suite ; sinon on attend le prochain, jusqu'au délai.
 *   identifiants  chemin ABSOLU d'un fichier qui contient « utilisateur:mot de
 *                 passe » sur sa première ligne. Le mot de passe n'est jamais
 *                 dans l'adresse : l'adresse est un argument de programme, que
 *                 tout utilisateur de la machine peut lire (ps). Le fichier est
 *                 refusé s'il n'appartient pas à l'utilisateur ou si un autre
 *                 peut le lire (chmod 600).
 *   confiance     mqtts:// seulement, OBLIGATOIRE : chemin absolu du certificat
 *                 (PEM) de l'autorité qui a signé le serveur. Aucun mode « tout
 *                 accepter » : sans lui, mqtts:// est refusé. Le nom du serveur
 *                 est vérifié contre son certificat.
 *   certificat, cle  mqtts:// seulement, ensemble : certificat et clé du poste,
 *                 pour un serveur qui en exige un. La clé est refusée si un
 *                 autre que l'utilisateur peut la lire (chmod 600).
 *   cache, perime comme sermo-source-modbus (commun.h).
 * Sortie : le contenu du message, suivi d'une fin de ligne ; code 0. Rien
 * reçu dans le délai, ou message vide (retenu effacé) : rien sur la sortie
 * standard, la raison sur la sortie d'erreur, code 1. Après un échec de
 * connexion, la même base répond sans réseau pendant 5 s (commun.h).
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <mosquitto.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "commun.h"
#include "mqtt-commun.h"

#define PROGRAMME "sermo-source-mqtt"
#define MESSAGE_MAX 65536

typedef enum { ATTENTE, CONNECTE, RECU, REFUSE } etat_t;

typedef struct {
	struct mosquitto *m;
	const char *base, *sujet;
	const mqtt_reglage_t *r;
	long delai;
	etat_t etat;
	int code_connack;          /* raison d'un refus du serveur */
	char *message;
	int taille;
} lecture_mqtt_t;

/* Le serveur chiffré a refusé la poignée de main, ou son certificat ne tient
 * pas : erreur de réglage, pas une panne réseau (pas de pause). */
static lecture_t
refus_tls(const lecture_mqtt_t *l)
{
	mqtt_dire_refus_tls(PROGRAMME, l->base);
	return LECTURE_REFUS;
}

static void
sur_connexion(struct mosquitto *m, void *donnees, int rc)
{
	lecture_mqtt_t *l = donnees;
	if (rc != 0) {
		l->code_connack = rc;
		l->etat = REFUSE;
		return;
	}
	l->etat = CONNECTE;
	if (mosquitto_subscribe(m, NULL, l->sujet, 0) != MOSQ_ERR_SUCCESS) {
		l->code_connack = -1;
		l->etat = REFUSE;
	}
}

static void
sur_message(struct mosquitto *m, void *donnees, const struct mosquitto_message *msg)
{
	(void)m;
	lecture_mqtt_t *l = donnees;
	if (l->etat == RECU || !msg || strcmp(msg->topic, l->sujet) != 0)
		return;
	l->taille = msg->payloadlen;
	if (msg->payloadlen > 0 && msg->payloadlen <= MESSAGE_MAX) {
		l->message = malloc((size_t)msg->payloadlen);
		if (l->message)
			memcpy(l->message, msg->payload, (size_t)msg->payloadlen);
	}
	l->etat = RECU;
}

/* Le lecteur de source_lire() (commun.h) : connexion, abonnement, un message. */
static lecture_t
lire_mqtt(void *donnees, FILE *sortie)
{
	lecture_mqtt_t *l = donnees;
	long fin = mqtt_maintenant_ms() + l->delai;
	int erreur, expire;

	l->etat = ATTENTE;
	int rc = mqtt_connecter(l->m, l->r, l->delai, &erreur, &expire);
	if (expire) {
		fprintf(stderr, PROGRAMME ": %s ne répond pas dans le délai (%ld ms)\n", l->base, l->delai);
		return LECTURE_PANNE;
	}
	if (mqtt_echec_tls(l->r->tls, rc, erreur))
		return refus_tls(l);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, PROGRAMME ": connexion à %s impossible (%s)\n", l->base, mqtt_raison(rc, erreur));
		return LECTURE_PANNE;
	}
	while (l->etat != RECU && l->etat != REFUSE) {
		long reste = fin - mqtt_maintenant_ms();
		if (reste <= 0)
			break;
		rc = mosquitto_loop(l->m, reste < 100 ? (int)reste : 100, 1);
		if (mqtt_echec_tls(l->r->tls, rc, errno) && l->etat == ATTENTE) {
			mosquitto_disconnect(l->m);
			return refus_tls(l);
		}
		if (rc != MOSQ_ERR_SUCCESS && l->etat == ATTENTE) {
			fprintf(stderr, PROGRAMME ": connexion à %s impossible (%s)\n", l->base,
			        rc == MOSQ_ERR_ERRNO ? strerror(errno) : mosquitto_strerror(rc));
			mosquitto_disconnect(l->m);
			return LECTURE_PANNE;
		}
	}
	mosquitto_disconnect(l->m);

	switch (l->etat) {
	case ATTENTE:
		fprintf(stderr, PROGRAMME ": %s ne répond pas dans le délai (%ld ms)\n", l->base, l->delai);
		return LECTURE_PANNE;
	case REFUSE:
		if (l->code_connack > 0)
			fprintf(stderr, PROGRAMME ": %s refuse la connexion (%s)\n", l->base,
			        mosquitto_connack_string(l->code_connack));
		else
			fprintf(stderr, PROGRAMME ": abonnement à « %s » refusé\n", l->sujet);
		return LECTURE_REFUS;
	case CONNECTE:
		fprintf(stderr, PROGRAMME ": aucun message sur « %s » dans le délai (%ld ms)\n", l->sujet, l->delai);
		return LECTURE_REFUS;
	case RECU:
		break;
	}
	if (l->taille <= 0) {
		fprintf(stderr, PROGRAMME ": message vide sur « %s »\n", l->sujet);
		return LECTURE_REFUS;
	}
	if (l->taille > MESSAGE_MAX || !l->message) {
		fprintf(stderr, PROGRAMME ": message trop long sur « %s » (%d octets, %d au plus)\n",
		        l->sujet, l->taille, MESSAGE_MAX);
		return LECTURE_REFUS;
	}
	fwrite(l->message, 1, (size_t)l->taille, sortie);
	if (l->message[l->taille - 1] != '\n')
		fputc('\n', sortie);
	return LECTURE_OK;
}

int
main(int argc, char **argv)
{
	static const char *const connues[] = { "sujet", "delai", "identifiants", "cache", "perime",
	                                       "confiance", "certificat", "cle", "clair", NULL };
	adresse_t a;

	/* Écrire sur une connexion chiffrée que le serveur a fermée tuerait le
	 * programme (SIGPIPE, code 141) après une lecture réussie. */
	signal(SIGPIPE, SIG_IGN);
	if (argc != 2) {
		fputs("usage : " PROGRAMME " ADRESSE   (appelé par <input mqtt>)\n", stderr);
		return 1;
	}
	if (adresse_lire(PROGRAMME, argv[1], &a) != 0)
		return 1;

	int code = 1, init = 0;
	long delai, cache, perime;
	char base[4096];
	const char *sujet = adresse_valeur(&a, "sujet");
	mqtt_reglage_t r;
	struct mosquitto *m = NULL;

	memset(&r, 0, sizeof r);
	snprintf(base, sizeof base, "%s", a.base);
	if (adresse_cles_connues(PROGRAMME, &a, connues) != 0)
		goto fin;
	if (mqtt_preparer(PROGRAMME, &a, &r) != 0)
		goto fin;
	if (!sujet || *sujet == '\0') {
		fputs(PROGRAMME ": sujet manquant (exemple : sujet=maison/salon/temperature)\n", stderr);
		goto fin;
	}
	if (strpbrk(sujet, "+#")) {
		fputs(PROGRAMME ": les jokers « + » et « # » sont refusés : un sujet précis\n", stderr);
		goto fin;
	}
	if (mosquitto_sub_topic_check(sujet) != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, PROGRAMME ": sujet invalide : %s\n", sujet);
		goto fin;
	}
	if (adresse_entier(PROGRAMME, &a, "delai", 10, 60000, 1500, &delai)
	    || adresse_entier(PROGRAMME, &a, "cache", 0, 60000, CACHE_DEFAUT_MS, &cache)
	    || adresse_entier(PROGRAMME, &a, "perime", 0, 600000, PERIME_DEFAUT_MS, &perime))
		goto fin;

	mosquitto_lib_init();
	init = 1;
	lecture_mqtt_t lecture = { NULL, base, sujet, &r, delai, ATTENTE, 0, NULL, 0 };
	m = mosquitto_new(NULL, true, &lecture);
	if (!m) {
		fprintf(stderr, PROGRAMME ": %s\n", strerror(errno));
		goto fin;
	}
	lecture.m = m;
	mosquitto_connect_callback_set(m, sur_connexion);
	mosquitto_message_callback_set(m, sur_message);
	if (mqtt_appliquer(PROGRAMME, m, &r) != 0)
		goto fin;
	code = source_lire(PROGRAMME, "mqtt", argv[1], base, cache, perime, lire_mqtt, &lecture);
	free(lecture.message);
fin:
	mqtt_reglage_liberer(&r);
	if (m)
		mosquitto_destroy(m);
	if (init)
		mosquitto_lib_cleanup();
	adresse_liberer(&a);
	return code;
}
