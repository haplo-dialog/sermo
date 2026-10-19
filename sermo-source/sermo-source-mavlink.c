/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo-source-mavlink — programme d'aide de <input mavlink> : lit UNE valeur de
 * télémétrie d'un drone (MAVLink 1 ou 2) et l'écrit sur la sortie standard.
 * Lecture SEULE : le programme n'ÉMET RIEN sur le lien — ni commande, ni
 * battement, ni demande de flux. Commander un drone sera le rôle d'un programme
 * à part, dans son propre paquet (sermo-cible-mavlink).
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Appelé par sermo (libsermocore/include/sermo_source.h) avec l'adresse
 * encodée en un argument :
 *   udp://[hôte]:port?champ=tension        écoute UDP (défaut 127.0.0.1 ; un
 *                                          relais local comme MAVProxy envoie là)
 *   tcp://hôte:port?champ=tension          connexion TCP (simulateur SITL : 5760)
 *   serie:///dev/ttyUSB0?champ=tension&vitesse=57600   radio USB
 * Clés :
 *   champ     obligatoire : tension (V), courant (A), batterie (%), latitude,
 *             longitude (degrés), altitude (m, niveau de la mer), hauteur (m,
 *             au-dessus du point de départ), cap (degrés), vitesse (m/s, sol),
 *             montee (m/s), satellites, fix (type de fix GPS), arme (1/0),
 *             mode (numéro de mode de l'autopilote)
 *   systeme   le système à lire (1 à 255) ; sans lui, le premier AUTOPILOTE
 *             entendu (les stations sol et autres composants sont ignorés)
 *   vitesse   débit de la liaison série (défaut 57600)
 *   delai, cache, perime   comme sermo-source-modbus (commun.h)
 * Sortie : la valeur, une ligne ; code 0. Rien reçu dans le délai, ou valeur
 * que l'autopilote dit « non mesurée » : rien sur la sortie standard, la raison
 * sur la sortie d'erreur, code 1.
 * Limite : n'émettant rien, le programme ne demande pas de flux ; sur un lien
 * qui ne diffuse rien d'office (pas de station sol ni de relais), seul le
 * battement (arme, mode) arrive.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/mavlink.h>

#include "commun.h"
#include "mavlink-commun.h"

#define PROGRAMME "sermo-source-mavlink"

typedef enum {
	C_TENSION, C_COURANT, C_BATTERIE, C_LATITUDE, C_LONGITUDE, C_ALTITUDE, C_HAUTEUR, C_CAP,
	C_VITESSE, C_MONTEE, C_SATELLITES, C_FIX, C_ARME, C_MODE
} champ_t;

static const char *const noms_champ[] = { "tension", "courant", "batterie", "latitude", "longitude",
	"altitude", "hauteur", "cap", "vitesse", "montee", "satellites", "fix", "arme", "mode", NULL };

typedef struct {
	lien_t lien;
	long delai, systeme;
	champ_t champ;
	const char *base;
} lecture_mavlink_t;

/* La valeur demandée, si @msg la porte. Rend 1 (écrite dans @texte), 0 (pas
 * dans ce message), -1 (dans ce message, mais « non mesurée »). */
static int
extraire(champ_t c, const mavlink_message_t *msg, char *texte, size_t n)
{
	switch (msg->msgid) {
	case MAVLINK_MSG_ID_HEARTBEAT:
		if (c == C_ARME) {
			snprintf(texte, n, "%d", (mavlink_msg_heartbeat_get_base_mode(msg) & MAV_MODE_FLAG_SAFETY_ARMED) ? 1 : 0);
			return 1;
		}
		if (c == C_MODE) {
			snprintf(texte, n, "%u", (unsigned)mavlink_msg_heartbeat_get_custom_mode(msg));
			return 1;
		}
		return 0;
	case MAVLINK_MSG_ID_SYS_STATUS:
		if (c == C_TENSION) {
			uint16_t mv = mavlink_msg_sys_status_get_voltage_battery(msg);
			if (mv == UINT16_MAX)
				return -1;
			snprintf(texte, n, "%.2f", mv / 1000.0);
			return 1;
		}
		if (c == C_COURANT) {
			int16_t ca = mavlink_msg_sys_status_get_current_battery(msg);
			if (ca == -1)
				return -1;
			snprintf(texte, n, "%.2f", ca / 100.0);
			return 1;
		}
		if (c == C_BATTERIE) {
			int8_t p = mavlink_msg_sys_status_get_battery_remaining(msg);
			if (p < 0)
				return -1;
			snprintf(texte, n, "%d", p);
			return 1;
		}
		return 0;
	case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
		switch (c) {
		case C_LATITUDE:  snprintf(texte, n, "%.7f", mavlink_msg_global_position_int_get_lat(msg) / 1e7); return 1;
		case C_LONGITUDE: snprintf(texte, n, "%.7f", mavlink_msg_global_position_int_get_lon(msg) / 1e7); return 1;
		case C_ALTITUDE:  snprintf(texte, n, "%.2f", mavlink_msg_global_position_int_get_alt(msg) / 1000.0); return 1;
		case C_HAUTEUR:   snprintf(texte, n, "%.2f", mavlink_msg_global_position_int_get_relative_alt(msg) / 1000.0); return 1;
		case C_CAP: {
			uint16_t h = mavlink_msg_global_position_int_get_hdg(msg);
			if (h == UINT16_MAX)
				return -1;
			snprintf(texte, n, "%.2f", h / 100.0);
			return 1;
		}
		default: return 0;
		}
	case MAVLINK_MSG_ID_GPS_RAW_INT:
		if (c == C_SATELLITES) {
			uint8_t s = mavlink_msg_gps_raw_int_get_satellites_visible(msg);
			if (s == UINT8_MAX)
				return -1;
			snprintf(texte, n, "%u", s);
			return 1;
		}
		if (c == C_FIX) {
			snprintf(texte, n, "%u", mavlink_msg_gps_raw_int_get_fix_type(msg));
			return 1;
		}
		return 0;
	case MAVLINK_MSG_ID_VFR_HUD:
		if (c == C_VITESSE) {
			snprintf(texte, n, "%.2f", mavlink_msg_vfr_hud_get_groundspeed(msg));
			return 1;
		}
		if (c == C_MONTEE) {
			snprintf(texte, n, "%.2f", mavlink_msg_vfr_hud_get_climb(msg));
			return 1;
		}
		return 0;
	default:
		return 0;
	}
}

/* Le lecteur de source_lire() (commun.h). */
static lecture_t
lire_mavlink(void *donnees, FILE *sortie)
{
	lecture_mavlink_t *l = donnees;
	long fin = mavlink_maintenant_ms() + l->delai;
	/* LECTURE : aucune écriture n'est jamais faite sur ce descripteur (la
	 * liaison série est même ouverte en lecture seule). */
	int fd = lien_ouvrir(PROGRAMME, &l->lien, fin, 0);
	if (fd < 0)
		return LECTURE_PANNE;

	mavlink_message_t msg;
	mavlink_status_t etat;
	memset(&etat, 0, sizeof etat);
	long systeme = l->systeme;            /* 0 : le premier autopilote entendu */
	int non_mesuree = 0, entendu = 0, vivant = 0;   /* vivant : un message MAVLink est arrivé */
	char texte[64];
	unsigned char tampon[2048];

	for (;;) {
		long reste = fin - mavlink_maintenant_ms();
		if (reste <= 0)
			break;
		struct pollfd p = { fd, POLLIN, 0 };
		int r = poll(&p, 1, (int)reste);
		if (r < 0 && errno == EINTR)
			continue;
		if (r <= 0)
			break;
		ssize_t n = read(fd, tampon, sizeof tampon);
		if (n == 0 && l->lien.transport != LIEN_UDP) {
			close(fd);
			fprintf(stderr, PROGRAMME ": %s : le lien s'est fermé\n", l->base);
			return LECTURE_PANNE;
		}
		if (n < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			close(fd);
			fprintf(stderr, PROGRAMME ": %s : lecture impossible (%s)\n", l->base, strerror(errno));
			return LECTURE_PANNE;
		}
		for (ssize_t i = 0; i < n; i++) {
			if (!mavlink_parse_char(MAVLINK_COMM_0, tampon[i], &msg, &etat))
				continue;
			vivant = 1;
			/* L'autopilote seulement : une station sol (255) ou un autre
			 * composant (caméra, nacelle…) ne parle pas pour le drone. */
			if (msg.compid != MAV_COMP_ID_AUTOPILOT1)
				continue;
			if (systeme == 0) {
				if (msg.msgid != MAVLINK_MSG_ID_HEARTBEAT
				    || mavlink_msg_heartbeat_get_autopilot(&msg) == MAV_AUTOPILOT_INVALID)
					continue;
				systeme = msg.sysid;
			}
			if (msg.sysid != systeme)
				continue;
			entendu = 1;
			int e = extraire(l->champ, &msg, texte, sizeof texte);
			if (e == 1) {
				close(fd);
				fprintf(sortie, "%s\n", texte);
				return LECTURE_OK;
			}
			if (e < 0)
				non_mesuree = 1;
		}
	}
	close(fd);
	if (non_mesuree)
		fprintf(stderr, PROGRAMME ": %s : l'autopilote dit « %s » non mesuré\n", l->base, noms_champ[l->champ]);
	else if (entendu)
		fprintf(stderr, PROGRAMME ": %s : le drone parle, mais « %s » n'est pas arrivé dans le délai (%ld ms)\n",
		        l->base, noms_champ[l->champ], l->delai);
	else if (vivant)
		/* Le lien parle, mais pas l'autopilote demandé : erreur de réglage
		 * (systeme=), pas une panne ; pas de pause pour les autres champs. */
		fprintf(stderr, PROGRAMME ": %s : aucun autopilote%s entendu dans le délai (%ld ms), le lien parle pourtant\n",
		        l->base, l->systeme ? " de ce système" : "", l->delai);
	else {
		fprintf(stderr, PROGRAMME ": %s : aucun autopilote entendu dans le délai (%ld ms), rien reçu\n", l->base, l->delai);
		return LECTURE_PANNE;
	}
	return LECTURE_REFUS;
}

int
main(int argc, char **argv)
{
	static const char *const connues[] = { "champ", "systeme", "vitesse", "delai", "cache", "perime", NULL };
	adresse_t a;

	signal(SIGPIPE, SIG_IGN);
	if (argc != 2) {
		fputs("usage : " PROGRAMME " ADRESSE   (appelé par <input mavlink>)\n", stderr);
		return 1;
	}
	if (adresse_lire(PROGRAMME, argv[1], &a) != 0)
		return 1;

	int code = 1;
	long cache, perime;
	char base[4096];
	lecture_mavlink_t l;
	memset(&l, 0, sizeof l);
	snprintf(base, sizeof base, "%s", a.base);
	l.base = base;
	const char *champ = adresse_valeur(&a, "champ");

	if (adresse_cles_connues(PROGRAMME, &a, connues) != 0)
		goto fin;
	if (lien_analyser(PROGRAMME, base, &l.lien) != 0)
		goto fin;
	if (!champ) {
		fputs(PROGRAMME ": champ manquant (exemple : champ=tension)\n", stderr);
		goto fin;
	}
	int i;
	for (i = 0; noms_champ[i] && strcmp(noms_champ[i], champ) != 0; i++)
		;
	if (!noms_champ[i]) {
		fprintf(stderr, PROGRAMME ": champ inconnu « %s » (tension, courant, batterie, latitude, longitude, "
		        "altitude, hauteur, cap, vitesse, montee, satellites, fix, arme, mode)\n", champ);
		goto fin;
	}
	l.champ = (champ_t)i;
	if (adresse_entier(PROGRAMME, &a, "systeme", 1, 255, 0, &l.systeme)
	    || adresse_entier(PROGRAMME, &a, "vitesse", 1200, 4000000, 57600, &l.lien.vitesse)
	    || adresse_entier(PROGRAMME, &a, "delai", 10, 60000, 1500, &l.delai)
	    || adresse_entier(PROGRAMME, &a, "cache", 0, 60000, CACHE_DEFAUT_MS, &cache)
	    || adresse_entier(PROGRAMME, &a, "perime", 0, 600000, PERIME_DEFAUT_MS, &perime))
		goto fin;
	if (adresse_valeur(&a, "vitesse") && l.lien.transport != LIEN_SERIE) {
		fputs(PROGRAMME ": vitesse ne vaut que pour serie://\n", stderr);
		goto fin;
	}
	if (l.lien.transport == LIEN_SERIE && mavlink_debit(l.lien.vitesse) == 0) {
		fputs(PROGRAMME ": vitesse vaut 9600, 19200, 38400, 57600, 115200, 230400, 460800 ou 921600\n", stderr);
		goto fin;
	}
	code = source_lire(PROGRAMME, "mavlink", argv[1], base, cache, perime, lire_mavlink, &l);
fin:
	adresse_liberer(&a);
	return code;
}
