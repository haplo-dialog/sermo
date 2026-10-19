/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo-cible-mqtt — programme d'ÉCRITURE de <action mqtt> : publie UNE valeur
 * sur UN sujet MQTT (domotique), si la liste blanche le permet.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Paquet à part (sermo-cible-mqtt) : qui n'installe que la lecture
 * (sermo-source-mqtt) ne peut rien publier. Le cœur ne l'appelle que depuis
 * <action mqtt> d'un bouton ou d'une entrée de menu, jamais depuis un <input>,
 * un minuteur ou un signal (refusé à l'analyse du XML).
 *
 * Adresse (un argument, encodée par le cœur comme pour <input mqtt>) :
 *   mqtt[s]://hôte[:port]?sujet=maison/salon/lampe&valeur=on[&clé=valeur…]
 * Clés :
 *   sujet         obligatoire, un sujet précis (jokers « + » « # » refusés)
 *   valeur        obligatoire : la valeur à publier, ou $NOM pour la variable
 *                 NOM du dialogue (sermo la pose dans l'environnement avant
 *                 d'exécuter l'action)
 *   delai         délai total en ms (connexion + accusé du serveur ; défaut
 *                 1500, de 10 à 60000)
 *   identifiants, confiance, certificat, cle : comme <input mqtt> (mqtt-commun.h)
 *
 * LISTE BLANCHE (fermée par défaut) : ~/.config/sermo/cibles-mqtt (à
 * l'utilisateur, lisible par lui seul) et /etc/sermo/cibles-mqtt (à root, que
 * personne d'autre ne peut écrire). Le dossier personnel vient de la base des
 * comptes, pas de $HOME : une variable posée par un script ne déplace pas la
 * liste. Un fichier mal protégé est refusé en entier, et c'est dit. Une ligne
 * par cible, « # » commente :
 *   <serveur> <sujet> <valeurs>
 *   mqtt://maison:1883 maison/salon/lampe on|off
 *   mqtts://maison maison/salon/consigne 18..24
 * Valeurs : « a|b|c » (liste fermée) ou « min..max » (nombre, point décimal,
 * bornes comprises). Le serveur se compare par chiffrement, hôte et port (ports
 * par défaut appliqués) ; le sujet, à l'identique.
 *
 * Publication en QoS 1 (le serveur accuse réception), non retenue ; une seule
 * tentative, jamais répétée. Code 0 : publié et accusé. Code 1 : rien publié,
 * la raison sur la sortie d'erreur.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <math.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "commun.h"
#include "mqtt-commun.h"

#define PROGRAMME "sermo-cible-mqtt"
#define LISTE_SYSTEME "/etc/sermo/cibles-mqtt"
#define LISTE_MAX 65536

typedef enum { ATTENTE, CONNECTE, PUBLIE, ACCUSE, REFUSE } etat_t;

typedef struct {
	const char *sujet, *valeur;
	etat_t etat;
	int code_connack;
	int mid;
} ecriture_t;

static void
sur_connexion(struct mosquitto *m, void *donnees, int rc)
{
	ecriture_t *e = donnees;
	if (rc != 0) {
		e->code_connack = rc;
		e->etat = REFUSE;
		return;
	}
	e->etat = CONNECTE;
	if (mosquitto_publish(m, &e->mid, e->sujet, (int)strlen(e->valeur), e->valeur, 1, false) != MOSQ_ERR_SUCCESS) {
		e->code_connack = -1;
		e->etat = REFUSE;
		return;
	}
	e->etat = PUBLIE;
}

static void
sur_accuse(struct mosquitto *m, void *donnees, int mid)
{
	(void)m;
	ecriture_t *e = donnees;
	if (e->etat == PUBLIE && mid == e->mid)
		e->etat = ACCUSE;
}

/* Nombre écrit avec un point, en entier ; NAN sinon. */
static double
nombre(const char *texte)
{
	char *fin;
	locale_t c = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
	if (!c)
		return NAN;
	errno = 0;
	double v = strtod_l(texte, &fin, c); /* garde:primitive-locale */
	freelocale(c);
	if (errno || fin == texte || *fin || !isfinite(v))
		return NAN;
	return v;
}

/* @valeur est-elle permise par @valeurs (« a|b » ou « min..max ») ? */
static int
valeur_permise(const char *valeurs, const char *valeur)
{
	const char *deux = strstr(valeurs, "..");
	if (deux) {
		char min[64], max[64];
		size_t n = (size_t)(deux - valeurs);
		if (n == 0 || n >= sizeof min || strlen(deux + 2) == 0 || strlen(deux + 2) >= sizeof max)
			return 0;
		memcpy(min, valeurs, n);
		min[n] = '\0';
		snprintf(max, sizeof max, "%s", deux + 2);
		double a = nombre(min), b = nombre(max), v = nombre(valeur);
		return !isnan(a) && !isnan(b) && !isnan(v) && a <= v && v <= b;
	}
	size_t lv = strlen(valeur);
	for (const char *p = valeurs; *p;) {
		const char *q = strchr(p, '|');
		size_t n = q ? (size_t)(q - p) : strlen(p);
		if (n == lv && strncmp(p, valeur, n) == 0)
			return 1;
		if (!q)
			break;
		p = q + 1;
	}
	return 0;
}

/* Une ligne de valeurs est-elle bien formée ? (liste non vide sans élément
 * vide, ou intervalle de deux nombres min <= max) */
static int
valeurs_bien_formees(const char *valeurs)
{
	const char *deux = strstr(valeurs, "..");
	if (deux) {
		char min[64];
		size_t n = (size_t)(deux - valeurs);
		if (n == 0 || n >= sizeof min)
			return 0;
		memcpy(min, valeurs, n);
		min[n] = '\0';
		double a = nombre(min), b = nombre(deux + 2);
		return !isnan(a) && !isnan(b) && a <= b;
	}
	if (*valeurs == '\0' || *valeurs == '|' || valeurs[strlen(valeurs) - 1] == '|' || strstr(valeurs, "||"))
		return 0;
	return 1;
}

/* Cherche la cible dans le fichier @chemin. Rend 1 (permise), 0 (absente du
 * fichier, ou fichier absent), -1 (fichier refusé ou mal formé : TOUT refuser).
 * @uid : propriétaire exigé ; @prive : personne d'autre ne doit pouvoir le
 * LIRE (sinon : personne d'autre ne doit pouvoir l'ÉCRIRE). */
static int
liste_chercher(const char *chemin, uid_t uid, int prive, int tls, const char *hote, int port,
               const char *sujet, const char *valeur, int *sujet_connu)
{
	struct stat st;
	int fd = open(chemin, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	if (fd < 0) {
		if (errno == ENOENT)
			return 0;
		fprintf(stderr, PROGRAMME ": liste blanche %s illisible (%s) : rien n'est publié\n", chemin, strerror(errno));
		return -1;
	}
	if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != uid
	    || (st.st_mode & (prive ? 077 : 022)) != 0) {
		fprintf(stderr, PROGRAMME ": liste blanche %s refusée : elle doit appartenir à %s et %s "
		        "(chmod %s) ; rien n'est publié\n", chemin, prive ? "vous" : "root",
		        prive ? "n'être lisible que par vous" : "ne pouvoir être modifiée que par root", prive ? "600" : "644");
		close(fd);
		return -1;
	}
	char *texte = malloc(LISTE_MAX + 1);
	if (!texte) {
		close(fd);
		return -1;
	}
	ssize_t n = read(fd, texte, LISTE_MAX + 1);
	close(fd);
	if (n < 0 || n > LISTE_MAX) {
		fprintf(stderr, PROGRAMME ": liste blanche %s illisible ou trop longue : rien n'est publié\n", chemin);
		free(texte);
		return -1;
	}
	texte[n] = '\0';

	int trouve = 0, numero = 0;
	char *suite = NULL;
	for (char *ligne = strtok_r(texte, "\n", &suite); ligne; ligne = strtok_r(NULL, "\n", &suite)) {
		numero++;
		char *diese = strchr(ligne, '#');
		if (diese)
			*diese = '\0';
		char *mots[4], *s2 = NULL;
		int k = 0;
		for (char *m = strtok_r(ligne, " \t\r", &s2); m && k < 4; m = strtok_r(NULL, " \t\r", &s2))
			mots[k++] = m;
		if (k == 0)
			continue;
		int ltls, lport;
		char lhote[1024];
		if (k != 3 || mqtt_base(mots[0], &ltls, lhote, sizeof lhote, &lport) != 0
		    || strpbrk(mots[1], "+#") || !valeurs_bien_formees(mots[2])) {
			fprintf(stderr, PROGRAMME ": liste blanche %s, ligne %d mal formée (« serveur sujet valeurs », "
			        "sans joker, valeurs « a|b » ou « min..max ») : rien n'est publié\n", chemin, numero);
			free(texte);
			return -1;
		}
		if (ltls == tls && lport == port && strcasecmp(lhote, hote) == 0 && strcmp(mots[1], sujet) == 0) {
			*sujet_connu = 1;
			if (valeur_permise(mots[2], valeur))
				trouve = 1;
		}
	}
	free(texte);
	return trouve;
}

int
main(int argc, char **argv)
{
	static const char *const connues[] = { "sujet", "valeur", "delai", "identifiants",
	                                       "confiance", "certificat", "cle", "clair", NULL };
	adresse_t a;

	signal(SIGPIPE, SIG_IGN);
	if (argc != 2) {
		fputs("usage : " PROGRAMME " ADRESSE   (appelé par <action mqtt>)\n", stderr);
		return 1;
	}
	if (adresse_lire(PROGRAMME, argv[1], &a) != 0)
		return 1;

	int code = 1, init = 0;
	long delai;
	const char *sujet = adresse_valeur(&a, "sujet");
	const char *brute = adresse_valeur(&a, "valeur");
	const char *valeur = NULL;
	mqtt_reglage_t r;
	struct mosquitto *m = NULL;

	memset(&r, 0, sizeof r);
	if (adresse_cles_connues(PROGRAMME, &a, connues) != 0 || mqtt_preparer(PROGRAMME, &a, &r) != 0)
		goto fin;
	if (!sujet || *sujet == '\0') {
		fputs(PROGRAMME ": sujet manquant (exemple : sujet=maison/salon/lampe)\n", stderr);
		goto fin;
	}
	if (strpbrk(sujet, "+#") || mosquitto_pub_topic_check(sujet) != MOSQ_ERR_SUCCESS) {
		fputs(PROGRAMME ": sujet refusé : un sujet précis, sans joker « + » ni « # »\n", stderr);
		goto fin;
	}
	if (!brute) {
		fputs(PROGRAMME ": valeur manquante (exemple : valeur=on, ou valeur=$NOM)\n", stderr);
		goto fin;
	}
	if (brute[0] == '$') {
		const char *nom = brute + 1;
		size_t i = 0;
		for (; nom[i]; i++)
			if (!((nom[i] >= 'A' && nom[i] <= 'Z') || (nom[i] >= 'a' && nom[i] <= 'z') || nom[i] == '_'
			      || (i > 0 && nom[i] >= '0' && nom[i] <= '9')))
				break;
		if (i == 0 || nom[i] != '\0') {
			fprintf(stderr, PROGRAMME ": valeur=%s : nom de variable invalide\n", brute);
			goto fin;
		}
		valeur = getenv(nom);
		if (!valeur) {
			fprintf(stderr, PROGRAMME ": valeur=%s : la variable %s n'existe pas dans ce dialogue\n", brute, nom);
			goto fin;
		}
		/* Une variable sur plusieurs lignes : la première seulement, comme
		 * les widgets ; jamais une valeur vide. */
		static char ligne[4096];
		snprintf(ligne, sizeof ligne, "%s", valeur);
		ligne[strcspn(ligne, "\r\n")] = '\0';
		valeur = ligne;
	} else {
		valeur = brute;
	}
	if (*valeur == '\0') {
		fputs(PROGRAMME ": valeur vide : rien n'est publié\n", stderr);
		goto fin;
	}
	if (adresse_entier(PROGRAMME, &a, "delai", 10, 60000, 1500, &delai) != 0)
		goto fin;

	/* La liste blanche : l'utilisateur, puis la machine. */
	struct passwd *pw = getpwuid(geteuid());
	char perso[4096];
	if (!pw || !pw->pw_dir || pw->pw_dir[0] != '/') {
		fputs(PROGRAMME ": dossier personnel introuvable : rien n'est publié\n", stderr);
		goto fin;
	}
	snprintf(perso, sizeof perso, "%s/.config/sermo/cibles-mqtt", pw->pw_dir);
	int connu = 0;
	int p1 = liste_chercher(perso, geteuid(), 1, r.tls, r.hote, r.port, sujet, valeur, &connu);
	if (p1 < 0)
		goto fin;
	int p2 = liste_chercher(LISTE_SYSTEME, 0, 0, r.tls, r.hote, r.port, sujet, valeur, &connu);
	if (p2 < 0)
		goto fin;
	if (!p1 && !p2) {
		if (connu)
			fprintf(stderr, PROGRAMME ": valeur « %s » non permise pour « %s » par la liste blanche : rien n'est publié\n",
			        valeur, sujet);
		else
			fprintf(stderr, PROGRAMME ": « %s » sur %s n'est pas dans la liste blanche (%s, %s) : rien n'est publié\n",
			        sujet, a.base, perso, LISTE_SYSTEME);
		goto fin;
	}

	mosquitto_lib_init();
	init = 1;
	ecriture_t e = { sujet, valeur, ATTENTE, 0, 0 };
	m = mosquitto_new(NULL, true, &e);
	if (!m) {
		fprintf(stderr, PROGRAMME ": %s\n", strerror(errno));
		goto fin;
	}
	mosquitto_connect_callback_set(m, sur_connexion);
	mosquitto_publish_callback_set(m, sur_accuse);
	if (mqtt_appliquer(PROGRAMME, m, &r) != 0)
		goto fin;

	long fin_ms = mqtt_maintenant_ms() + delai;
	int erreur, expire;
	int rc = mqtt_connecter(m, &r, delai, &erreur, &expire);
	if (expire) {
		fprintf(stderr, PROGRAMME ": %s ne répond pas dans le délai (%ld ms) : rien n'est publié\n", a.base, delai);
		goto fin;
	}
	if (mqtt_echec_tls(r.tls, rc, erreur)) {
		mqtt_dire_refus_tls(PROGRAMME, a.base);
		goto fin;
	}
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, PROGRAMME ": connexion à %s impossible (%s) : rien n'est publié\n", a.base, mqtt_raison(rc, erreur));
		goto fin;
	}
	while (e.etat != ACCUSE && e.etat != REFUSE) {
		long reste = fin_ms - mqtt_maintenant_ms();
		if (reste <= 0)
			break;
		rc = mosquitto_loop(m, reste < 100 ? (int)reste : 100, 1);
		if (rc != MOSQ_ERR_SUCCESS && e.etat == ATTENTE) {
			if (mqtt_echec_tls(r.tls, rc, errno))
				mqtt_dire_refus_tls(PROGRAMME, a.base);
			else
				fprintf(stderr, PROGRAMME ": connexion à %s perdue (%s)\n", a.base, mqtt_raison(rc, errno));
			goto fin;
		}
	}
	mosquitto_disconnect(m);
	mosquitto_loop(m, 50, 1);
	switch (e.etat) {
	case ACCUSE:
		code = 0;
		break;
	case REFUSE:
		if (e.code_connack > 0)
			fprintf(stderr, PROGRAMME ": %s refuse la connexion (%s)\n", a.base, mosquitto_connack_string(e.code_connack));
		else
			fprintf(stderr, PROGRAMME ": publication sur « %s » refusée\n", sujet);
		break;
	case PUBLIE:
		/* Envoyé, mais l'accusé n'est pas venu : on ne sait pas si le serveur
		 * l'a reçu. On le dit, sans réessayer (jamais de répétition). */
		fprintf(stderr, PROGRAMME ": « %s » envoyé, mais le serveur n'a pas accusé réception dans le délai\n", sujet);
		break;
	default:
		fprintf(stderr, PROGRAMME ": %s ne répond pas dans le délai (%ld ms) : rien n'est publié\n", a.base, delai);
		break;
	}
fin:
	mqtt_reglage_liberer(&r);
	if (m)
		mosquitto_destroy(m);
	if (init)
		mosquitto_lib_cleanup();
	adresse_liberer(&a);
	return code;
}
