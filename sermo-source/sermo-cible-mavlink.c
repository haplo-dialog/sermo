/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo-cible-mavlink — programme d'ÉCRITURE de <action mavlink> : règle UN
 * paramètre d'un drone au sol, ou lui donne UN ordre de sécurité (retour au
 * départ, atterrissage, coupure des moteurs), si la liste blanche le permet.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Paquet à part (sermo-cible-mavlink) : qui n'installe que la lecture
 * (sermo-source-mavlink) ne peut rien envoyer. Le cœur ne l'appelle que depuis
 * <action mavlink> d'un bouton ou d'une entrée de menu (refusé ailleurs à
 * l'analyse du XML).
 *
 * Adresse (un argument, encodée par le cœur ; liaisons de <input mavlink>) :
 *   tcp://127.0.0.1:5760?parametre=WPNAV_SPEED&valeur=500
 *   udp://:14550?ordre=rtl
 * Clés :
 *   parametre   nom exact d'un paramètre de l'autopilote (A-Z, 0-9, _ ; 16 au
 *               plus), avec valeur= (un nombre, ou $NOM : la variable NOM du
 *               dialogue)
 *   ordre       rtl (retour au départ), land (atterrir sur place) ou
 *               terminaison (couper les moteurs : DEUX appuis, voir plus bas)
 *   mission     chemin absolu d'un fichier « QGC WPL 110 » (Mission Planner,
 *               QGroundControl) : des POINTS DE PASSAGE seulement
 *               (mission-wpl.h), envoyés au drone désarmé puis RELUS et
 *               comparés ; la mission n'est pas lancée
 *   systeme     le système à commander (1 à 255) ; sans lui, le premier
 *               autopilote entendu
 *   vitesse     débit de la liaison série (défaut 57600)
 *   delai       délai total en ms (défaut 5000, 20000 pour une mission ; de
 *               2000 à 60000)
 *
 * LISTE BLANCHE (fermée par défaut) : ~/.config/sermo/cibles-mavlink (à
 * l'utilisateur, lisible par lui seul) et /etc/sermo/cibles-mavlink (à root,
 * que personne d'autre ne peut écrire), comme pour MQTT. Une ligne par
 * permission, « # » commente :
 *   parametre WPNAV_SPEED 100..1500
 *   ordre rtl
 *   ordre land
 *   ordre terminaison
 *   mission rayon 500 hauteur 120
 * La ligne mission borne chaque point : à R mètres au plus de la position
 * actuelle du drone, de 0 à H mètres de hauteur (H : 120 au plus). Deux
 * lignes : la plus stricte. Les familles ARMING_*, FLTMODE*, SERVOn_FUNCTION, BRD_SAFETY*, FENCE_* sont
 * TOUJOURS refusées : dans la liste (le fichier entier est alors refusé, et
 * c'est dit) comme dans une adresse.
 *
 * DÉROULÉ, une seule tentative, jamais répétée :
 *  1. le premier battement de l'autopilote attendu, une demande de LECTURE
 *     envoyée (le premier paramètre : ArduPilot ne relaie les messages des
 *     autres liens que vers un lien qui lui a déjà parlé), puis écoute
 *     ECOUTE_MS : si une station sol parle déjà (battement de type GCS),
 *     refus — un seul émetteur ;
 *  2. paramètre : lecture de sa valeur et de son type (un paramètre que
 *     l'autopilote ne connaît pas est refusé) ;
 *  3. attente du battement SUIVANT de l'autopilote, et envoi tout de suite
 *     après (état lu moins de 200 ms avant) ; un paramètre n'est réglé que si
 *     ce battement dit « désarmé » ;
 *  4. succès seulement sur la preuve du drone : PARAM_VALUE renvoyant la même
 *     valeur, ou COMMAND_ACK « accepté ».
 * sermo parle comme système 255 (celui qu'ArduPilot accepte d'une station sol
 * quand MAV_OPTIONS l'exige), composant 25 : PAS 190, celui des stations sol
 * usuelles — ArduPilot ne renvoie jamais les battements d'une identité vers le
 * lien d'où elle parle, et sermo, sous la même identité, ne les entendrait
 * pas. Il n'envoie JAMAIS de
 * battement : ArduPilot compte le battement d'une station sol pour sa sécurité
 * « perte de la station sol » ; un battement, puis plus rien quand ce
 * programme se termine, pourrait la déclencher en vol.
 *
 * COUPURE DES MOTEURS : le premier appui n'envoie rien ; il pose une marque
 * datée (dossier $XDG_RUNTIME_DIR/sermo-cible-mavlink, à vous seul) et le dit.
 * Un second appui dans les CONFIRMATION_MS envoie l'ordre ; après, la marque
 * est périmée et l'on recommence.
 *
 * MESSAGE : chaque appel écrit sa conclusion, une ligne, sur la sortie
 * d'erreur ET dans $XDG_RUNTIME_DIR/sermo-cible-mavlink/message : un dialogue
 * peut la montrer (<input file> et refresh: après l'action).
 * Code 0 : envoyé et confirmé par le drone. Code 1 : sinon.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <math.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <common/mavlink.h>

#include "commun.h"
#include "mavlink-commun.h"
#include "mission-wpl.h"

#define PROGRAMME "sermo-cible-mavlink"
#define LISTE_SYSTEME "/etc/sermo/cibles-mavlink"
#define LISTE_MAX 65536
#define ECOUTE_MS 1100            /* une station sol bat à 1 Hz */
#define CONFIRMATION_MS 5000
#define RAYON_MAX 10000.0
#define HAUTEUR_MAX 120.0
#define MON_SYSTEME 255
#define MON_COMPOSANT MAV_COMP_ID_USER1   /* 25 (voir plus haut) */

typedef enum { O_AUCUN, O_RTL, O_LAND, O_TERMINAISON } ordre_t;
static const char *const noms_ordre[] = { "", "rtl", "land", "terminaison" };

/* ---- message ------------------------------------------------------------ */

static char dossier_etat[4096];

/* La conclusion : sortie d'erreur, et fichier « message » (écrit en place
 * par un fichier temporaire renommé). */
static void __attribute__((format(printf, 1, 2)))
conclure(const char *format, ...)
{
	char texte[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(texte, sizeof texte, format, ap);
	va_end(ap);
	fprintf(stderr, PROGRAMME ": %s\n", texte);
	if (!dossier_etat[0])
		return;
	char tmp[4200], fin[4200];
	snprintf(tmp, sizeof tmp, "%s/message.%ld", dossier_etat, (long)getpid());
	snprintf(fin, sizeof fin, "%s/message", dossier_etat);
	int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (fd < 0)
		return;
	FILE *f = fdopen(fd, "w");
	if (!f) {
		close(fd);
		unlink(tmp);
		return;
	}
	fprintf(f, "%s\n", texte);
	if (fclose(f) != 0 || rename(tmp, fin) != 0)
		unlink(tmp);
}

/* $XDG_RUNTIME_DIR/sermo-cible-mavlink, à nous, 700 ; vide si impossible. */
static void
dossier_preparer(void)
{
	const char *x = getenv("XDG_RUNTIME_DIR");
	struct stat st;
	if (!x || x[0] != '/')
		return;
	snprintf(dossier_etat, sizeof dossier_etat, "%s/sermo-cible-mavlink", x);
	if (mkdir(dossier_etat, 0700) != 0 && errno != EEXIST) {
		dossier_etat[0] = '\0';
		return;
	}
	if (lstat(dossier_etat, &st) != 0 || !S_ISDIR(st.st_mode) || st.st_uid != geteuid() || (st.st_mode & 077))
		dossier_etat[0] = '\0';
}

/* ---- nombres, familles, liste blanche ------------------------------------ */

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

static int
nom_parametre_valide(const char *nom)
{
	size_t n = strlen(nom);
	if (n == 0 || n > MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN)
		return 0;
	for (size_t i = 0; i < n; i++)
		if (!((nom[i] >= 'A' && nom[i] <= 'Z') || (nom[i] >= '0' && nom[i] <= '9') || nom[i] == '_'))
			return 0;
	return 1;
}

/* Familles qui arment, changent les modes, réaffectent les sorties moteur,
 * désactivent l'interrupteur de sécurité ou la barrière : jamais réglées par
 * sermo. Rend le nom de la famille, ou NULL. */
static const char *
famille_interdite(const char *nom)
{
	static const char *const prefixes[] = { "ARMING_", "FLTMODE", "BRD_SAFETY", "FENCE_", NULL };
	for (int i = 0; prefixes[i]; i++)
		if (strncmp(nom, prefixes[i], strlen(prefixes[i])) == 0)
			return prefixes[i];
	if (strncmp(nom, "SERVO", 5) == 0) {
		const char *p = nom + 5;
		while (*p >= '0' && *p <= '9')
			p++;
		if (p > nom + 5 && strcmp(p, "_FUNCTION") == 0)
			return "SERVOn_FUNCTION";
	}
	return NULL;
}

/* « min..max », deux nombres, min <= max. */
static int
intervalle(const char *texte, double *min, double *max)
{
	const char *deux = strstr(texte, "..");
	char a[64];
	size_t n = deux ? (size_t)(deux - texte) : 0;
	if (!deux || n == 0 || n >= sizeof a)
		return 0;
	memcpy(a, texte, n);
	a[n] = '\0';
	*min = nombre(a);
	*max = nombre(deux + 2);
	return !isnan(*min) && !isnan(*max) && *min <= *max;
}

/* Cherche la permission dans @chemin. Rend 1 (permise), 0 (absente, ou
 * fichier absent), -1 (fichier refusé ou mal formé : TOUT refuser). Pour un
 * paramètre connu mais hors de son intervalle, *connu = 1. */
static int
liste_chercher(const char *chemin, uid_t uid, int prive, const char *parametre, double valeur,
               ordre_t ordre, int mission, double *rayon, double *hauteur, int *connu)
{
	struct stat st;
	int fd = open(chemin, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	if (fd < 0) {
		if (errno == ENOENT)
			return 0;
		conclure("liste blanche %s illisible (%s) : rien n'est envoyé", chemin, strerror(errno));
		return -1;
	}
	if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != uid
	    || (st.st_mode & (prive ? 077 : 022)) != 0) {
		conclure("liste blanche %s refusée : elle doit appartenir à %s et %s (chmod %s) ; rien n'est envoyé",
		         chemin, prive ? "vous" : "root",
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
		conclure("liste blanche %s illisible ou trop longue : rien n'est envoyé", chemin);
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
		char *mots[6], *s2 = NULL;
		int k = 0;
		for (char *m = strtok_r(ligne, " \t\r", &s2); m && k < 6; m = strtok_r(NULL, " \t\r", &s2))
			mots[k++] = m;
		if (k == 0)
			continue;
		double min, max;
		if (k == 3 && strcmp(mots[0], "parametre") == 0 && nom_parametre_valide(mots[1])
		    && intervalle(mots[2], &min, &max)) {
			const char *f = famille_interdite(mots[1]);
			if (f) {
				conclure("liste blanche %s, ligne %d : %s appartient à la famille %s, jamais réglée par "
				         "sermo ; rien n'est envoyé", chemin, numero, mots[1], f);
				free(texte);
				return -1;
			}
			if (parametre && strcmp(mots[1], parametre) == 0) {
				*connu = 1;
				if (min <= valeur && valeur <= max)
					trouve = 1;
			}
			continue;
		}
		if (k == 2 && strcmp(mots[0], "ordre") == 0
		    && (strcmp(mots[1], "rtl") == 0 || strcmp(mots[1], "land") == 0 || strcmp(mots[1], "terminaison") == 0)) {
			if (ordre != O_AUCUN && strcmp(mots[1], noms_ordre[ordre]) == 0)
				trouve = 1;
			continue;
		}
		if (k == 5 && strcmp(mots[0], "mission") == 0 && strcmp(mots[1], "rayon") == 0
		    && strcmp(mots[3], "hauteur") == 0) {
			double r = nombre(mots[2]), h = nombre(mots[4]);
			if (!isnan(r) && !isnan(h) && r >= 1 && r <= RAYON_MAX && h >= 1 && h <= HAUTEUR_MAX) {
				if (mission) {
					*rayon = isnan(*rayon) ? r : fmin(*rayon, r);
					*hauteur = isnan(*hauteur) ? h : fmin(*hauteur, h);
					trouve = 1;
				}
				continue;
			}
		}
		conclure("liste blanche %s, ligne %d mal formée (« parametre NOM min..max », « ordre rtl|land|"
		         "terminaison » ou « mission rayon 1..10000 hauteur 1..120 ») : rien n'est envoyé", chemin, numero);
		free(texte);
		return -1;
	}
	free(texte);
	return trouve;
}

/* ---- double appui -------------------------------------------------------- */

/* Nom de la marque de @base/@systeme : empreinte FNV-1a, le texte de
 * l'adresse ne devient jamais un chemin. */
static void
marque_chemin(char *chemin, size_t n, const char *base, long systeme)
{
	uint64_t h = 1469598103934665603ULL;
	for (const char *p = base; *p; p++)
		h = (h ^ (unsigned char)*p) * 1099511628211ULL;
	h = (h ^ (uint64_t)systeme) * 1099511628211ULL;
	snprintf(chemin, n, "%s/terminaison-%016llx", dossier_etat, (unsigned long long)h);
}

/* Heure de la marque : CLOCK_BOOTTIME, en ms (la même que /proc/uptime ; une
 * mise en veille entre deux appuis compte comme du temps écoulé). */
static long
marque_heure(void)
{
	struct timespec t;
	clock_gettime(CLOCK_BOOTTIME, &t);
	return (long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

/* Rend 1 si une marque FRAÎCHE existait (elle est effacée : on peut envoyer),
 * 0 si elle vient d'être posée (premier appui), -1 si impossible. */
static int
marque_confirmer(const char *base, long systeme)
{
	char chemin[4300];
	if (!dossier_etat[0])
		return -1;
	marque_chemin(chemin, sizeof chemin, base, systeme);
	long maintenant = marque_heure();
	int fd = open(chemin, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	if (fd >= 0) {
		char t[32] = "";
		ssize_t n = read(fd, t, sizeof t - 1);
		struct stat st;
		int sure = fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_uid == geteuid() && !(st.st_mode & 077);
		close(fd);
		unlink(chemin);
		if (n > 0 && sure) {
			t[n] = '\0';
			char *fin;
			long pose = strtol(t, &fin, 10);
			if (*fin == '\n' && pose <= maintenant && maintenant - pose <= CONFIRMATION_MS)
				return 1;
		}
	}
	fd = open(chemin, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (fd < 0)
		return -1;
	char t[32];
	int n = snprintf(t, sizeof t, "%ld\n", maintenant);
	int ok = write(fd, t, (size_t)n) == n;
	close(fd);
	return ok ? 0 : -1;
}

/* ---- lien ---------------------------------------------------------------- */

typedef struct {
	const lien_t *lien;
	int fd;
	struct sockaddr_storage pair;     /* UDP : d'où parle l'autopilote */
	socklen_t pair_n;
	long systeme;                     /* 0 : le premier autopilote entendu */
	mavlink_status_t etat;
	int autre_station;                /* système de la station sol entendue, ou 0 */
	int position;                     /* une position de l'autopilote est arrivée */
	double latitude, longitude;
	int vivant;                       /* un message MAVLink est arrivé */
	unsigned char tampon[2048];
	ssize_t n, i;
} canal_t;

/* Le message suivant, avant @fin. Rend 1 (dans @msg), 0 (délai), -1 (lien
 * fermé ou en panne, message dit). Toute station sol entendue est notée. */
static int
recevoir(canal_t *c, mavlink_message_t *msg, long fin)
{
	for (;;) {
		while (c->i < c->n) {
			if (!mavlink_parse_char(MAVLINK_COMM_0, c->tampon[c->i++], msg, &c->etat))
				continue;
			c->vivant = 1;
			if (msg->msgid == MAVLINK_MSG_ID_HEARTBEAT && mavlink_msg_heartbeat_get_type(msg) == MAV_TYPE_GCS)
				c->autre_station = msg->sysid ? msg->sysid : 256;
			if (msg->msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT && msg->compid == MAV_COMP_ID_AUTOPILOT1
			    && c->systeme != 0 && msg->sysid == c->systeme) {
				c->latitude = mavlink_msg_global_position_int_get_lat(msg) / 1e7;
				c->longitude = mavlink_msg_global_position_int_get_lon(msg) / 1e7;
				c->position = !(c->latitude == 0 && c->longitude == 0);
			}
			return 1;
		}
		long reste = fin - mavlink_maintenant_ms();
		if (reste <= 0)
			return 0;
		struct pollfd p = { c->fd, POLLIN, 0 };
		int r = poll(&p, 1, (int)reste);
		if (r < 0 && errno == EINTR)
			continue;
		if (r <= 0)
			return 0;
		struct sockaddr_storage de;
		socklen_t den = sizeof de;
		ssize_t n = c->lien->transport == LIEN_UDP
		            ? recvfrom(c->fd, c->tampon, sizeof c->tampon, 0, (struct sockaddr *)&de, &den)
		            : read(c->fd, c->tampon, sizeof c->tampon);
		if (n == 0 && c->lien->transport != LIEN_UDP) {
			conclure("%s : le lien s'est fermé ; rien n'est confirmé", c->lien->base);
			return -1;
		}
		if (n < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			conclure("%s : lecture impossible (%s)", c->lien->base, strerror(errno));
			return -1;
		}
		if (c->lien->transport == LIEN_UDP) {
			memcpy(&c->pair, &de, den);
			c->pair_n = den;
		}
		c->n = n;
		c->i = 0;
	}
}

/* Le message vient-il de l'autopilote suivi ? (adopte le premier entendu) */
static int
de_l_autopilote(canal_t *c, const mavlink_message_t *msg)
{
	if (msg->compid != MAV_COMP_ID_AUTOPILOT1)
		return 0;
	if (c->systeme == 0) {
		if (msg->msgid != MAVLINK_MSG_ID_HEARTBEAT || mavlink_msg_heartbeat_get_autopilot(msg) == MAV_AUTOPILOT_INVALID)
			return 0;
		c->systeme = msg->sysid;
	}
	return msg->sysid == c->systeme;
}

static int
envoyer(canal_t *c, const mavlink_message_t *msg)
{
	uint8_t octets[MAVLINK_MAX_PACKET_LEN];
	uint16_t n = mavlink_msg_to_send_buffer(octets, msg);
	ssize_t e;
	if (c->lien->transport == LIEN_UDP) {
		if (c->pair_n == 0)
			return -1;
		e = sendto(c->fd, octets, n, 0, (struct sockaddr *)&c->pair, c->pair_n);
	} else {
		e = write(c->fd, octets, n);
	}
	return e == (ssize_t)n ? 0 : -1;
}

static const char *
resultat(uint8_t r)
{
	switch (r) {
	case MAV_RESULT_ACCEPTED: return "accepté";
	case MAV_RESULT_TEMPORARILY_REJECTED: return "refusé pour l'instant";
	case MAV_RESULT_DENIED: return "refusé";
	case MAV_RESULT_UNSUPPORTED: return "non pris en charge";
	case MAV_RESULT_FAILED: return "échoué";
	default: return "réponse inconnue";
	}
}

static int
type_entier(uint8_t t)
{
	return t != MAV_PARAM_TYPE_REAL32 && t != MAV_PARAM_TYPE_REAL64;
}

static int
meme_valeur(float a, double b)
{
	return fabs((double)a - b) <= 1e-6 * fmax(1.0, fabs(b));
}

/* ---- mission ------------------------------------------------------------- */

static const char *
resultat_mission(uint8_t r)
{
	switch (r) {
	case MAV_MISSION_ACCEPTED: return "acceptée";
	case MAV_MISSION_NO_SPACE: return "refusée : plus de place sur le drone";
	case MAV_MISSION_INVALID_SEQUENCE: return "refusée : élément hors séquence";
	case MAV_MISSION_DENIED: return "refusée : le drone n'accepte pas de mission de ce partenaire";
	case MAV_MISSION_OPERATION_CANCELLED: return "annulée par le drone";
	default: return "refusée par le drone";
	}
}

/* Demande d'élément du drone (MISSION_REQUEST_INT, ou MISSION_REQUEST des
 * anciens autopilotes) pour la mission principale : rend 1 et @seq. */
static int
demande_element(const mavlink_message_t *msg, uint16_t *seq)
{
	if (msg->msgid == MAVLINK_MSG_ID_MISSION_REQUEST_INT
	    && mavlink_msg_mission_request_int_get_mission_type(msg) == MAV_MISSION_TYPE_MISSION) {
		*seq = mavlink_msg_mission_request_int_get_seq(msg);
		return 1;
	}
	if (msg->msgid == MAVLINK_MSG_ID_MISSION_REQUEST
	    && mavlink_msg_mission_request_get_mission_type(msg) == MAV_MISSION_TYPE_MISSION) {
		*seq = mavlink_msg_mission_request_get_seq(msg);
		return 1;
	}
	return 0;
}

static int32_t
en_e7(double degres)
{
	return (int32_t)lround(degres * 1e7);
}

/* L'élément @seq : 0 = le « départ » (position actuelle, ArduPilot le
 * remplace par le sien), puis les points. */
static void
element(const canal_t *c, const mission_t *m, uint16_t seq, mavlink_message_t *msg)
{
	if (seq == 0) {
		mavlink_msg_mission_item_int_pack(MON_SYSTEME, MON_COMPOSANT, msg, (uint8_t)c->systeme, MAV_COMP_ID_AUTOPILOT1,
		                                  0, MAV_FRAME_GLOBAL, MAV_CMD_NAV_WAYPOINT, 0, 1, 0, 0, 0, 0,
		                                  en_e7(c->latitude), en_e7(c->longitude), 0, MAV_MISSION_TYPE_MISSION);
		return;
	}
	const point_t *p = &m->point[seq - 1];
	mavlink_msg_mission_item_int_pack(MON_SYSTEME, MON_COMPOSANT, msg, (uint8_t)c->systeme, MAV_COMP_ID_AUTOPILOT1,
	                                  seq, MAV_FRAME_GLOBAL_RELATIVE_ALT, MAV_CMD_NAV_WAYPOINT, 0, 1,
	                                  (float)p->attente, 0, 0, 0, en_e7(p->latitude), en_e7(p->longitude),
	                                  (float)p->hauteur, MAV_MISSION_TYPE_MISSION);
}

/* Envoie la mission puis la RELIT et la compare. Rend 0 (identique), 1 sinon
 * (raison dite). */
static int
mission_envoyer(canal_t *c, const mission_t *m, long fin_ms, const char *base)
{
	mavlink_message_t msg;
	uint16_t total = (uint16_t)(m->n + 1), seq;
	int r, servis = 0;
	char vu[MISSION_POINTS_MAX + 1];
	memset(vu, 0, sizeof vu);

	mavlink_msg_mission_count_pack(MON_SYSTEME, MON_COMPOSANT, &msg, (uint8_t)c->systeme, MAV_COMP_ID_AUTOPILOT1,
	                               total, MAV_MISSION_TYPE_MISSION, 0);
	if (envoyer(c, &msg) != 0) {
		conclure("%s : envoi impossible : rien n'est envoyé", base);
		return 1;
	}
	for (;;) {
		r = recevoir(c, &msg, fin_ms);
		if (r < 0)
			return 1;
		if (r == 0) {
			conclure("mission : le drone s'est tu pendant l'envoi (%d éléments sur %u demandés) ; pas de nouvel "
			         "essai", servis, total);
			return 1;
		}
		if (!de_l_autopilote(c, &msg))
			continue;
		if (demande_element(&msg, &seq)) {
			if (seq >= total) {
				conclure("mission : le drone demande l'élément %u, hors de la mission (%u)", seq, total);
				return 1;
			}
			element(c, m, seq, &msg);
			if (envoyer(c, &msg) != 0) {
				conclure("%s : envoi impossible pendant la mission", base);
				return 1;
			}
			if (!vu[seq]) {
				vu[seq] = 1;
				servis++;
			}
			continue;
		}
		if (msg.msgid == MAVLINK_MSG_ID_MISSION_ACK
		    && mavlink_msg_mission_ack_get_mission_type(&msg) == MAV_MISSION_TYPE_MISSION) {
			uint8_t res = mavlink_msg_mission_ack_get_type(&msg);
			if (res != MAV_MISSION_ACCEPTED) {
				conclure("mission %s (code %u)", resultat_mission(res), res);
				return 1;
			}
			if (servis != total) {
				conclure("mission : le drone a conclu après %d éléments sur %u", servis, total);
				return 1;
			}
			break;
		}
	}

	/* La preuve : relire toute la mission et la comparer. */
	mavlink_msg_mission_request_list_pack(MON_SYSTEME, MON_COMPOSANT, &msg, (uint8_t)c->systeme,
	                                      MAV_COMP_ID_AUTOPILOT1, MAV_MISSION_TYPE_MISSION);
	if (envoyer(c, &msg) != 0) {
		conclure("%s : relecture impossible", base);
		return 1;
	}
	int compte = -1;
	while (compte < 0 && (r = recevoir(c, &msg, fin_ms)) == 1)
		if (de_l_autopilote(c, &msg) && msg.msgid == MAVLINK_MSG_ID_MISSION_COUNT
		    && mavlink_msg_mission_count_get_mission_type(&msg) == MAV_MISSION_TYPE_MISSION)
			compte = mavlink_msg_mission_count_get_count(&msg);
	if (compte < 0) {
		if (r == 0)
			conclure("mission envoyée, mais le drone n'a pas répondu à la relecture ; non confirmée");
		return 1;
	}
	if (compte != total) {
		conclure("mission envoyée, mais le drone en garde %d éléments au lieu de %u", compte, total);
		return 1;
	}
	for (uint16_t s = 1; s < total; s++) {
		mavlink_msg_mission_request_int_pack(MON_SYSTEME, MON_COMPOSANT, &msg, (uint8_t)c->systeme,
		                                     MAV_COMP_ID_AUTOPILOT1, s, MAV_MISSION_TYPE_MISSION);
		if (envoyer(c, &msg) != 0) {
			conclure("%s : relecture impossible", base);
			return 1;
		}
		int lu = 0;
		while (!lu && (r = recevoir(c, &msg, fin_ms)) == 1) {
			if (!de_l_autopilote(c, &msg) || msg.msgid != MAVLINK_MSG_ID_MISSION_ITEM_INT
			    || mavlink_msg_mission_item_int_get_seq(&msg) != s)
				continue;
			lu = 1;
			const point_t *p = &m->point[s - 1];
			uint8_t repere = mavlink_msg_mission_item_int_get_frame(&msg);
			if (mavlink_msg_mission_item_int_get_command(&msg) != MAV_CMD_NAV_WAYPOINT
			    || (repere != MAV_FRAME_GLOBAL_RELATIVE_ALT && repere != MAV_FRAME_GLOBAL_RELATIVE_ALT_INT)
			    || mavlink_msg_mission_item_int_get_x(&msg) != en_e7(p->latitude)
			    || mavlink_msg_mission_item_int_get_y(&msg) != en_e7(p->longitude)
			    || fabs(mavlink_msg_mission_item_int_get_z(&msg) - p->hauteur) > 0.01
			    || fabs(mavlink_msg_mission_item_int_get_param1(&msg) - p->attente) > 0.01) {
				conclure("mission envoyée, mais le point %u relu diffère (%.7f, %.7f, %.2f m) : NON confirmée", s,
				         mavlink_msg_mission_item_int_get_x(&msg) / 1e7, mavlink_msg_mission_item_int_get_y(&msg) / 1e7,
				         (double)mavlink_msg_mission_item_int_get_z(&msg));
				return 1;
			}
		}
		if (!lu) {
			if (r == 0)
				conclure("mission envoyée, mais le point %u n'a pas pu être relu ; non confirmée", s);
			return 1;
		}
	}
	/* Fin de la relecture, comme le veut le protocole. */
	mavlink_msg_mission_ack_pack(MON_SYSTEME, MON_COMPOSANT, &msg, (uint8_t)c->systeme, MAV_COMP_ID_AUTOPILOT1,
	                             MAV_MISSION_ACCEPTED, MAV_MISSION_TYPE_MISSION, 0);
	envoyer(c, &msg);
	conclure("mission de %d point%s envoyée, relue et identique sur le drone (non lancée)", m->n, m->n > 1 ? "s" : "");
	return 0;
}

/* Lit le fichier de mission : à vous, non modifiable par d'autres, pas un
 * lien, 256 Kio au plus. */
static int
mission_charger(const char *chemin, mission_t *m)
{
	struct stat st;
	char raison[256];
	if (chemin[0] != '/') {
		conclure("mission=%s : un chemin absolu est attendu ; rien n'est envoyé", chemin);
		return -1;
	}
	int fd = open(chemin, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	if (fd < 0) {
		conclure("mission %s illisible (%s) ; rien n'est envoyé", chemin, strerror(errno));
		return -1;
	}
	if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != geteuid() || (st.st_mode & 022)
	    || st.st_size > MISSION_OCTETS_MAX) {
		conclure("mission %s refusée : elle doit être un fichier à vous, que personne d'autre ne peut modifier, "
		         "de 256 Kio au plus ; rien n'est envoyé", chemin);
		close(fd);
		return -1;
	}
	char *texte = malloc(MISSION_OCTETS_MAX + 2);
	if (!texte) {
		close(fd);
		return -1;
	}
	ssize_t n = read(fd, texte, MISSION_OCTETS_MAX + 1);
	close(fd);
	if (n < 0 || n > MISSION_OCTETS_MAX || memchr(texte, '\0', (size_t)n)) {
		conclure("mission %s illisible, trop longue ou binaire ; rien n'est envoyé", chemin);
		free(texte);
		return -1;
	}
	texte[n] = '\0';
	int e = mission_lire(texte, m, raison, sizeof raison);
	free(texte);
	if (e) {
		conclure("mission %s refusée : %s ; rien n'est envoyé", chemin, raison);
		return -1;
	}
	return 0;
}

int
main(int argc, char **argv)
{
	static const char *const connues[] = { "parametre", "valeur", "ordre", "mission", "systeme", "vitesse", "delai",
	                                       NULL };
	static mission_t mission;
	adresse_t a;

	signal(SIGPIPE, SIG_IGN);
	if (argc != 2) {
		fputs("usage : " PROGRAMME " ADRESSE   (appelé par <action mavlink>)\n", stderr);
		return 1;
	}
	dossier_preparer();
	if (adresse_lire(PROGRAMME, argv[1], &a) != 0)
		return 1;

	int code = 1;
	long delai, systeme;
	char base[4096];
	lien_t lien;
	canal_t c;
	memset(&lien, 0, sizeof lien);
	memset(&c, 0, sizeof c);
	c.fd = -1;
	snprintf(base, sizeof base, "%s", a.base);
	const char *parametre = adresse_valeur(&a, "parametre");
	const char *brute = adresse_valeur(&a, "valeur");
	const char *nom_ordre = adresse_valeur(&a, "ordre");
	const char *chemin_mission = adresse_valeur(&a, "mission");
	ordre_t ordre = O_AUCUN;
	double rayon = NAN, hauteur = NAN;
	double valeur = NAN;

	if (adresse_cles_connues(PROGRAMME, &a, connues) != 0 || lien_analyser(PROGRAMME, base, &lien) != 0)
		goto fin;
	if (adresse_entier(PROGRAMME, &a, "systeme", 1, 255, 0, &systeme)
	    || adresse_entier(PROGRAMME, &a, "vitesse", 1200, 4000000, 57600, &lien.vitesse)
	    || adresse_entier(PROGRAMME, &a, "delai", 2000, 60000, chemin_mission ? 20000 : 5000, &delai))
		goto fin;
	if (adresse_valeur(&a, "vitesse") && lien.transport != LIEN_SERIE) {
		fputs(PROGRAMME ": vitesse ne vaut que pour serie://\n", stderr);
		goto fin;
	}
	if (lien.transport == LIEN_SERIE && mavlink_debit(lien.vitesse) == 0) {
		fputs(PROGRAMME ": vitesse vaut 9600, 19200, 38400, 57600, 115200, 230400, 460800 ou 921600\n", stderr);
		goto fin;
	}
	if (!!parametre + !!nom_ordre + !!chemin_mission != 1) {
		conclure("donner parametre= (avec valeur=), ordre= OU mission= : rien n'est envoyé");
		goto fin;
	}
	if (chemin_mission) {
		if (brute) {
			conclure("valeur= ne va qu'avec parametre= : rien n'est envoyé");
			goto fin;
		}
		if (mission_charger(chemin_mission, &mission) != 0)
			goto fin;
	} else if (nom_ordre) {
		for (int i = O_RTL; i <= O_TERMINAISON; i++)
			if (strcmp(nom_ordre, noms_ordre[i]) == 0)
				ordre = (ordre_t)i;
		if (ordre == O_AUCUN) {
			conclure("ordre inconnu « %s » (rtl, land ou terminaison) : rien n'est envoyé", nom_ordre);
			goto fin;
		}
		if (brute) {
			conclure("valeur= ne va qu'avec parametre= : rien n'est envoyé");
			goto fin;
		}
	} else {
		if (!nom_parametre_valide(parametre)) {
			conclure("nom de paramètre invalide « %s » (A-Z, 0-9, _ ; 16 au plus) : rien n'est envoyé", parametre);
			goto fin;
		}
		const char *f = famille_interdite(parametre);
		if (f) {
			conclure("%s appartient à la famille %s, jamais réglée par sermo : rien n'est envoyé", parametre, f);
			goto fin;
		}
		if (!brute) {
			conclure("valeur manquante (exemple : valeur=500, ou valeur=$NOM) : rien n'est envoyé");
			goto fin;
		}
		const char *texte = brute;
		char ligne[256];
		if (brute[0] == '$') {
			const char *nom = brute + 1;
			size_t i = 0;
			for (; nom[i]; i++)
				if (!((nom[i] >= 'A' && nom[i] <= 'Z') || (nom[i] >= 'a' && nom[i] <= 'z') || nom[i] == '_'
				      || (i > 0 && nom[i] >= '0' && nom[i] <= '9')))
					break;
			if (i == 0 || nom[i] != '\0') {
				conclure("valeur=%s : nom de variable invalide : rien n'est envoyé", brute);
				goto fin;
			}
			const char *v = getenv(nom);
			if (!v) {
				conclure("valeur=%s : la variable %s n'existe pas dans ce dialogue : rien n'est envoyé", brute, nom);
				goto fin;
			}
			snprintf(ligne, sizeof ligne, "%s", v);
			ligne[strcspn(ligne, "\r\n")] = '\0';
			texte = ligne;
		}
		valeur = nombre(texte);
		if (isnan(valeur)) {
			conclure("valeur « %s » : un nombre attendu (point décimal) : rien n'est envoyé", texte);
			goto fin;
		}
	}

	/* La liste blanche : l'utilisateur, puis la machine. */
	struct passwd *pw = getpwuid(geteuid());
	char perso[4096];
	if (!pw || !pw->pw_dir || pw->pw_dir[0] != '/') {
		conclure("dossier personnel introuvable : rien n'est envoyé");
		goto fin;
	}
	snprintf(perso, sizeof perso, "%s/.config/sermo/cibles-mavlink", pw->pw_dir);
	int connu = 0;
	int p1 = liste_chercher(perso, geteuid(), 1, parametre, valeur, ordre, !!chemin_mission, &rayon, &hauteur, &connu);
	if (p1 < 0)
		goto fin;
	int p2 = liste_chercher(LISTE_SYSTEME, 0, 0, parametre, valeur, ordre, !!chemin_mission, &rayon, &hauteur, &connu);
	if (p2 < 0)
		goto fin;
	if (!p1 && !p2) {
		if (connu)
			conclure("valeur %g hors de l'intervalle permis pour %s par la liste blanche : rien n'est envoyé",
			         valeur, parametre);
		else if (parametre)
			conclure("le paramètre %s n'est pas dans la liste blanche (%s, %s) : rien n'est envoyé",
			         parametre, perso, LISTE_SYSTEME);
		else if (chemin_mission)
			conclure("les missions ne sont pas dans la liste blanche (ligne « mission rayon R hauteur H » dans %s "
			         "ou %s) : rien n'est envoyé", perso, LISTE_SYSTEME);
		else
			conclure("l'ordre %s n'est pas dans la liste blanche (%s, %s) : rien n'est envoyé",
			         noms_ordre[ordre], perso, LISTE_SYSTEME);
		goto fin;
	}

	/* Mission : la hauteur de chaque point, avant tout réseau. */
	for (int i = 0; chemin_mission && i < mission.n; i++)
		if (mission.point[i].hauteur < 0 || mission.point[i].hauteur > hauteur) {
			conclure("mission : le point %d est à %g m de hauteur, hors de 0..%g m permis par la liste blanche ; "
			         "rien n'est envoyé", i + 1, mission.point[i].hauteur, hauteur);
			goto fin;
		}

	/* Coupure des moteurs : deux appuis. */
	if (ordre == O_TERMINAISON) {
		int m = marque_confirmer(base, systeme);
		if (m < 0) {
			conclure("coupure des moteurs : la confirmation demande $XDG_RUNTIME_DIR (à vous seul) ; "
			         "rien n'est envoyé");
			goto fin;
		}
		if (m == 0) {
			conclure("COUPURE DES MOTEURS : appuyer encore dans les %d s pour confirmer (le drone tombera) ; "
			         "rien n'est envoyé", CONFIRMATION_MS / 1000);
			goto fin;
		}
	}

	long fin_ms = mavlink_maintenant_ms() + delai;
	c.lien = &lien;
	c.systeme = systeme;
	c.fd = lien_ouvrir(PROGRAMME, &lien, fin_ms, 1);
	if (c.fd < 0) {
		conclure("%s injoignable : rien n'est envoyé", base);
		goto fin;
	}
	mavlink_message_t msg;
	int r, battu = 0, deja_arme = 0;

	/* 1. Le premier battement de l'autopilote (son système, et en UDP son
	 * adresse), une demande de lecture, puis l'écoute : un seul émetteur ; et
	 * l'état armé, avant de rien envoyer qui écrive. */
	while (!battu && (r = recevoir(&c, &msg, fin_ms)) == 1)
		if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT && de_l_autopilote(&c, &msg)) {
			battu = 1;
			deja_arme = (mavlink_msg_heartbeat_get_base_mode(&msg) & MAV_MODE_FLAG_SAFETY_ARMED) != 0;
		}
	if (r < 0)
		goto fin;
	if (!battu) {
		conclure("%s : aucun autopilote%s entendu dans le délai (%ld ms) : rien n'est envoyé", base,
		         systeme ? " de ce système" : "", delai);
		goto fin;
	}
	mavlink_msg_param_request_read_pack(MON_SYSTEME, MON_COMPOSANT, &msg, (uint8_t)c.systeme,
	                                    MAV_COMP_ID_AUTOPILOT1, "", 0);
	if (envoyer(&c, &msg) != 0) {
		conclure("%s : envoi impossible : rien n'est envoyé", base);
		goto fin;
	}
	long ecoute = mavlink_maintenant_ms() + ECOUTE_MS;
	while ((r = recevoir(&c, &msg, ecoute < fin_ms ? ecoute : fin_ms)) == 1)
		if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT && de_l_autopilote(&c, &msg))
			deja_arme = (mavlink_msg_heartbeat_get_base_mode(&msg) & MAV_MODE_FLAG_SAFETY_ARMED) != 0;
	if (r < 0)
		goto fin;
	if (c.autre_station) {
		conclure("une station sol parle déjà à ce drone (système %d) : un seul émetteur, rien n'est envoyé",
		         c.autre_station == 256 ? 0 : c.autre_station);
		goto fin;
	}

	if ((parametre || chemin_mission) && deja_arme) {
		conclure("le drone est ARMÉ : %s qu'au sol, désarmé ; rien n'est envoyé",
		         parametre ? "un paramètre ne se règle" : "une mission ne s'envoie");
		goto fin;
	}

	/* Mission : la distance de chaque point à la position actuelle du drone. */
	if (chemin_mission) {
		while (!c.position && (r = recevoir(&c, &msg, fin_ms)) == 1)
			;
		if (r < 0)
			goto fin;
		if (!c.position) {
			conclure("mission : le drone ne donne pas sa position (sur ArduPilot 4.7 : MAV1_POSITION) ; la distance "
			         "des points ne peut pas être vérifiée, rien n'est envoyé");
			goto fin;
		}
		for (int i = 0; i < mission.n; i++) {
			double d = mission_distance(c.latitude, c.longitude, mission.point[i].latitude, mission.point[i].longitude);
			if (d > rayon) {
				conclure("mission : le point %d est à %.0f m du drone, au-delà des %g m permis par la liste blanche ; "
				         "rien n'est envoyé", i + 1, d, rayon);
				goto fin;
			}
		}
	}

	/* 2. Paramètre : il existe, et son type. */
	uint8_t type = MAV_PARAM_TYPE_REAL32;
	char id[MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN + 1];
	memset(id, 0, sizeof id);
	if (parametre) {
		memcpy(id, parametre, strlen(parametre));
		mavlink_msg_param_request_read_pack(MON_SYSTEME, MON_COMPOSANT, &msg, (uint8_t)c.systeme,
		                                    MAV_COMP_ID_AUTOPILOT1, id, -1);
		if (envoyer(&c, &msg) != 0) {
			conclure("%s : envoi impossible : rien n'est envoyé", base);
			goto fin;
		}
		long attente = mavlink_maintenant_ms() + 1500;
		int lu = 0;
		while (!lu && (r = recevoir(&c, &msg, attente < fin_ms ? attente : fin_ms)) == 1) {
			if (msg.msgid != MAVLINK_MSG_ID_PARAM_VALUE || !de_l_autopilote(&c, &msg))
				continue;
			char recu[MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1];
			memset(recu, 0, sizeof recu);
			mavlink_msg_param_value_get_param_id(&msg, recu);
			if (strcmp(recu, parametre) == 0) {
				type = mavlink_msg_param_value_get_param_type(&msg);
				lu = 1;
			}
		}
		if (r < 0)
			goto fin;
		if (!lu) {
			conclure("%s : l'autopilote ne connaît pas le paramètre %s (aucune réponse) : rien n'est envoyé",
			         base, parametre);
			goto fin;
		}
		if (type_entier(type) && valeur != floor(valeur)) {
			conclure("%s est un entier : %g refusé, rien n'est envoyé", parametre, valeur);
			goto fin;
		}
	}

	/* 3. Le battement suivant, puis l'envoi tout de suite. */
	int arme = -1;
	while (arme < 0 && (r = recevoir(&c, &msg, fin_ms)) == 1)
		if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT && de_l_autopilote(&c, &msg))
			arme = (mavlink_msg_heartbeat_get_base_mode(&msg) & MAV_MODE_FLAG_SAFETY_ARMED) ? 1 : 0;
	if (r < 0)
		goto fin;
	if (c.autre_station) {
		conclure("une station sol parle déjà à ce drone (système %d) : un seul émetteur, rien n'est envoyé",
		         c.autre_station == 256 ? 0 : c.autre_station);
		goto fin;
	}
	if (arme < 0) {
		conclure("%s : l'autopilote s'est tu avant l'envoi (délai %ld ms) : rien n'est envoyé", base, delai);
		goto fin;
	}
	uint16_t commande = 0;
	if (chemin_mission) {
		if (arme) {
			conclure("le drone est ARMÉ : une mission ne s'envoie qu'au sol, désarmé ; rien n'est envoyé");
			goto fin;
		}
		code = mission_envoyer(&c, &mission, fin_ms, base);
		goto fin;
	}
	if (parametre) {
		if (arme) {
			conclure("le drone est ARMÉ : un paramètre ne se règle qu'au sol, désarmé ; rien n'est envoyé");
			goto fin;
		}
		mavlink_msg_param_set_pack(MON_SYSTEME, MON_COMPOSANT, &msg, (uint8_t)c.systeme, MAV_COMP_ID_AUTOPILOT1,
		                           id, (float)valeur, type);
	} else {
		commande = ordre == O_RTL ? MAV_CMD_NAV_RETURN_TO_LAUNCH
		           : ordre == O_LAND ? MAV_CMD_NAV_LAND : MAV_CMD_DO_FLIGHTTERMINATION;
		mavlink_msg_command_long_pack(MON_SYSTEME, MON_COMPOSANT, &msg, (uint8_t)c.systeme, MAV_COMP_ID_AUTOPILOT1,
		                              commande, 0, ordre == O_TERMINAISON ? 1.0f : 0.0f, 0, 0, 0, 0, 0, 0);
	}
	if (envoyer(&c, &msg) != 0) {
		conclure("%s : envoi impossible : rien n'est envoyé", base);
		goto fin;
	}

	/* 4. La preuve du drone. */
	while ((r = recevoir(&c, &msg, fin_ms)) == 1) {
		if (!de_l_autopilote(&c, &msg))
			continue;
		if (parametre && msg.msgid == MAVLINK_MSG_ID_PARAM_VALUE) {
			char recu[MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1];
			memset(recu, 0, sizeof recu);
			mavlink_msg_param_value_get_param_id(&msg, recu);
			if (strcmp(recu, parametre) != 0)
				continue;
			float v = mavlink_msg_param_value_get_param_value(&msg);
			if (meme_valeur(v, valeur)) {
				conclure("%s = %g : réglé et relu sur le drone", parametre, valeur);
				code = 0;
			} else {
				conclure("%s : le drone a gardé %g au lieu de %g", parametre, (double)v, valeur);
			}
			goto fin;
		}
		if (!parametre && msg.msgid == MAVLINK_MSG_ID_COMMAND_ACK
		    && mavlink_msg_command_ack_get_command(&msg) == commande) {
			uint8_t res = mavlink_msg_command_ack_get_result(&msg);
			if (res == MAV_RESULT_IN_PROGRESS)
				continue;
			if (res == MAV_RESULT_ACCEPTED) {
				conclure("ordre %s : accepté par le drone", noms_ordre[ordre]);
				code = 0;
			} else {
				conclure("ordre %s : %s par le drone", noms_ordre[ordre], resultat(res));
			}
			goto fin;
		}
	}
	if (r == 0)
		conclure("%s envoyé, mais le drone n'a pas confirmé dans le délai (%ld ms) ; pas de nouvel essai",
		         parametre ? parametre : noms_ordre[ordre], delai);
fin:
	if (c.fd >= 0)
		close(c.fd);
	adresse_liberer(&a);
	return code;
}
