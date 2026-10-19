/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mqtt-commun.c — partagé par sermo-source-mqtt et sermo-cible-mqtt.
 * Contrat : mqtt-commun.h. Repris tel quel de sermo-source-mqtt,
 * où chaque règle a son banc.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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

#include "mqtt-commun.h"

#define IDENTIFIANTS_MAX 4096

long
mqtt_maintenant_ms(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

/* mqtt[s]://hôte[:port] : hôte (nom, IPv4 ou [IPv6]) et port, découpés dans
 * @base après le préfixe de @prefixe octets ; @defaut si le port manque. */
static int
hote_port(char *base, size_t prefixe, int defaut, char **hote, int *port)
{
	char *h = base + prefixe, *p = NULL;
	if (*h == '[') {
		char *f = strchr(h, ']');
		if (!f || (f[1] != '\0' && f[1] != ':'))
			return -1;
		*f = '\0';
		if (f[1] == ':')
			p = f + 2;
		*hote = h + 1;
	} else {
		char *deux = strrchr(h, ':');
		if (deux) {
			*deux = '\0';
			p = deux + 1;
		}
		*hote = h;
	}
	if (**hote == '\0' || strchr(*hote, '/'))
		return -1;
	*port = defaut;
	if (p) {
		char *fin;
		long v = strtol(p, &fin, 10);
		if (*p == '\0' || *fin || v < 1 || v > 65535)
			return -1;
		*port = (int)v;
	}
	return 0;
}

/* Un fichier de certificat : chemin absolu, fichier ordinaire lisible. */
static int
fichier_lisible(const char *programme, const char *cle, const char *chemin)
{
	struct stat st;
	if (chemin[0] != '/') {
		fprintf(stderr, "%s: %s doit être un chemin absolu\n", programme, cle);
		return -1;
	}
	if (stat(chemin, &st) != 0 || !S_ISREG(st.st_mode) || access(chemin, R_OK) != 0) {
		fprintf(stderr, "%s: %s : fichier illisible ou absent (%s)\n", programme, cle, chemin);
		return -1;
	}
	return 0;
}

/* Une clé privée : en plus, à l'utilisateur et lisible par lui seul. */
static int
fichier_prive(const char *programme, const char *cle, const char *chemin)
{
	struct stat st;
	if (fichier_lisible(programme, cle, chemin) != 0)
		return -1;
	if (lstat(chemin, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != geteuid() || (st.st_mode & 077) != 0) {
		fprintf(stderr, "%s: %s refusée : elle doit être à vous et lisible par vous seul (chmod 600)\n", programme, cle);
		return -1;
	}
	return 0;
}

/* Lit « utilisateur:mot de passe » dans @chemin, après avoir vérifié que seul
 * l'utilisateur peut le lire. Rend 0 et deux chaînes allouées, ou -1. */
static int
identifiants_lire(const char *programme, const char *chemin, char **utilisateur, char **mot)
{
	char tampon[IDENTIFIANTS_MAX + 1];
	struct stat st;
	int fd;

	if (chemin[0] != '/') {
		fprintf(stderr, "%s: identifiants doit être un chemin absolu\n", programme);
		return -1;
	}
	fd = open(chemin, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "%s: fichier d'identifiants illisible (%s)\n", programme, strerror(errno));
		return -1;
	}
	if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
		fprintf(stderr, "%s: le fichier d'identifiants n'est pas un fichier ordinaire\n", programme);
		close(fd);
		return -1;
	}
	if (st.st_uid != geteuid() || (st.st_mode & 077) != 0) {
		fprintf(stderr, "%s: fichier d'identifiants refusé : il doit être à vous et lisible par vous seul (chmod 600)\n", programme);
		close(fd);
		return -1;
	}
	ssize_t n = read(fd, tampon, IDENTIFIANTS_MAX);
	close(fd);
	if (n <= 0) {
		fprintf(stderr, "%s: fichier d'identifiants vide\n", programme);
		return -1;
	}
	tampon[n] = '\0';
	tampon[strcspn(tampon, "\r\n")] = '\0';
	char *deux = strchr(tampon, ':');
	if (!deux || deux == tampon) {
		fprintf(stderr, "%s: fichier d'identifiants : « utilisateur:mot de passe » attendu sur la première ligne\n", programme);
		explicit_bzero(tampon, sizeof tampon);
		return -1;
	}
	*deux = '\0';
	*utilisateur = strdup(tampon);
	*mot = strdup(deux + 1);
	explicit_bzero(tampon, sizeof tampon);
	if (!*utilisateur || !*mot) {
		free(*utilisateur);
		free(*mot);
		*utilisateur = *mot = NULL;
		return -1;
	}
	return 0;
}

int
mqtt_base(const char *base, int *tls, char *hote, size_t n, int *port)
{
	char copie[4096], *h;
	*tls = strncmp(base, "mqtts://", 8) == 0;
	if (!*tls && strncmp(base, "mqtt://", 7) != 0)
		return -1;
	snprintf(copie, sizeof copie, "%s", base);
	if (hote_port(copie, *tls ? 8 : 7, *tls ? 8883 : 1883, &h, port) != 0 || strlen(h) >= n)
		return -1;
	snprintf(hote, n, "%s", h);
	return 0;
}

int
mqtt_preparer(const char *programme, const adresse_t *a, mqtt_reglage_t *r)
{
	const char *chemin_id = adresse_valeur(a, "identifiants");

	memset(r, 0, sizeof *r);
	r->confiance = adresse_valeur(a, "confiance");
	r->certificat = adresse_valeur(a, "certificat");
	r->cle = adresse_valeur(a, "cle");
	if (strncmp(a->base, "mqtts://", 8) == 0)
		r->tls = 1;
	else if (strncmp(a->base, "mqtt://", 7) != 0) {
		fprintf(stderr, "%s: l'adresse commence par mqtt:// ou mqtts://\n", programme);
		return -1;
	}
	snprintf(r->hotes, sizeof r->hotes, "%s", a->base);
	if (hote_port(r->hotes, r->tls ? 8 : 7, r->tls ? 8883 : 1883, &r->hote, &r->port) != 0) {
		fprintf(stderr, "%s: adresse attendue : mqtt[s]://hôte[:port]?sujet=…\n", programme);
		return -1;
	}
	if (!r->tls && (r->confiance || r->certificat || r->cle)) {
		fprintf(stderr, "%s: confiance, certificat et cle ne valent qu'avec mqtts://\n", programme);
		return -1;
	}
	if (r->tls) {
		if (!r->confiance) {
			fprintf(stderr, "%s: mqtts:// exige confiance=/chemin/autorite.pem (le certificat de l'autorité "
			        "qui a signé le serveur) ; aucun mode « tout accepter »\n", programme);
			return -1;
		}
		if (fichier_lisible(programme, "confiance", r->confiance) != 0)
			return -1;
		if (!r->certificat != !r->cle) {
			fprintf(stderr, "%s: certificat et cle vont ensemble\n", programme);
			return -1;
		}
		if (r->certificat && (fichier_lisible(programme, "certificat", r->certificat) != 0
		                      || fichier_prive(programme, "cle", r->cle) != 0))
			return -1;
	}
	const char *clair = adresse_valeur(a, "clair");
	if (clair && strcmp(clair, "oui") != 0) {
		fprintf(stderr, "%s: clair= n'admet que « oui »\n", programme);
		return -1;
	}
	if (clair && r->tls) {
		fprintf(stderr, "%s: clair=oui ne vaut qu'avec mqtt:// (mqtts:// est chiffré)\n", programme);
		return -1;
	}
	/* Un mot de passe sur mqtt:// passe en clair sur le réseau : refusé, sauf
	 * si l'auteur du dialogue l'écrit lui-même (réseau local de confiance). */
	if (chemin_id && !r->tls && !clair) {
		fprintf(stderr, "%s: identifiants refusés sur mqtt:// : ils passeraient en clair sur le réseau ; "
		        "utilisez mqtts://, ou ajoutez clair=oui pour un réseau local de confiance\n", programme);
		return -1;
	}
	if (chemin_id && identifiants_lire(programme, chemin_id, &r->utilisateur, &r->mot) != 0)
		return -1;
	return 0;
}

void
mqtt_reglage_liberer(mqtt_reglage_t *r)
{
	if (r->mot) {
		explicit_bzero(r->mot, strlen(r->mot));
		free(r->mot);
	}
	free(r->utilisateur);
	r->mot = r->utilisateur = NULL;
}

int
mqtt_appliquer(const char *programme, struct mosquitto *m, const mqtt_reglage_t *r)
{
	if (r->tls) {
		/* Vérification du certificat ET du nom du serveur ; jamais « insecure ». */
		if (mosquitto_tls_set(m, r->confiance, NULL, r->certificat, r->cle, NULL) != MOSQ_ERR_SUCCESS
		    || mosquitto_tls_opts_set(m, 1, NULL, NULL) != MOSQ_ERR_SUCCESS
		    || mosquitto_tls_insecure_set(m, false) != MOSQ_ERR_SUCCESS) {
			fprintf(stderr, "%s: réglage du chiffrement refusé (fichiers de certificat ou de clé invalides ?)\n", programme);
			return -1;
		}
	}
	if (r->utilisateur && mosquitto_username_pw_set(m, r->utilisateur, r->mot) != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "%s: identifiants refusés par la bibliothèque\n", programme);
		return -1;
	}
	return 0;
}

/* La connexion est BLOQUANTE, pour que libmosquitto essaie chaque adresse du
 * nom (« localhost » : ::1 puis 127.0.0.1) et fasse la poignée de main
 * chiffrée ; la version non bloquante n'essaie que la première adresse (vu le
 * 2026-10-17 : « localhost » refusé, serveur en IPv4 seul). Une minuterie
 * l'interrompt au bout du délai : un hôte qui ne répond pas ne fige rien. */
static void
reveil(int signal)
{
	(void)signal;
}

static void
minuterie(long ms)
{
	struct itimerval t = { { 0, 0 }, { ms / 1000, (ms % 1000) * 1000 } };
	setitimer(ITIMER_REAL, &t, NULL);
}

int
mqtt_connecter(struct mosquitto *m, const mqtt_reglage_t *r, long delai, int *erreur, int *expire)
{
	long fin = mqtt_maintenant_ms() + delai;
	struct sigaction sa;
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = reveil;          /* sans SA_RESTART : connect() rend EINTR */
	sigaction(SIGALRM, &sa, NULL);
	minuterie(delai);
	int rc = mosquitto_connect(m, r->hote, r->port, 30);
	*erreur = errno;
	minuterie(0);
	*expire = mqtt_maintenant_ms() >= fin || (rc == MOSQ_ERR_ERRNO && *erreur == EINTR);
	return rc;
}

/* Échec de la poignée de main chiffrée : libmosquitto le rend en MOSQ_ERR_TLS,
 * MOSQ_ERR_PROTOCOL, ou MOSQ_ERR_ERRNO avec errno = EPROTO (mesuré le
 * 2026-10-17 : certificat signé par une autre autorité). En TLS 1.3, un
 * serveur qui exige un certificat du poste ne le refuse qu'APRÈS la poignée de
 * main : la connexion se ferme avant la réponse au CONNECT (MOSQ_ERR_CONN_LOST,
 * mesuré le même jour). */
int
mqtt_echec_tls(int tls, int rc, int erreur)
{
	return rc == MOSQ_ERR_TLS
	       || (tls && (rc == MOSQ_ERR_PROTOCOL || rc == MOSQ_ERR_CONN_LOST
	                   || (rc == MOSQ_ERR_ERRNO && erreur == EPROTO)));
}

void
mqtt_dire_refus_tls(const char *programme, const char *base)
{
	fprintf(stderr, "%s: %s : échange chiffré refusé (certificat du serveur non signé par "
	        "l'autorité de confiance, nom de serveur différent, certificat expiré, ou certificat du poste exigé ou refusé)\n",
	        programme, base);
}

/* Après plusieurs adresses essayées, errno ne dit plus rien d'utile (EBADF) :
 * on le dit simplement. */
const char *
mqtt_raison(int rc, int erreur)
{
	if (rc != MOSQ_ERR_ERRNO)
		return mosquitto_strerror(rc);
	return (erreur == 0 || erreur == EBADF) ? "serveur absent ou injoignable" : strerror(erreur);
}
