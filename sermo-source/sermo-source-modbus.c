/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo-source-modbus — programme d'aide de <input modbus> : lit des registres
 * d'un automate Modbus (TCP ou RTU) et écrit leurs valeurs, une par ligne.
 * Lecture SEULE : seules les fonctions de lecture de libmodbus sont appelées
 * (codes 1, 2, 3 et 4) ; aucune écriture n'existe dans ce programme.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Appelé par sermo (voir libsermocore/include/sermo_source.h) avec l'adresse
 * encodée en un argument :
 *   tcp://hôte[:port]?registre=1[&clé=valeur…]       (port 502 par défaut)
 *   rtu:///dev/ttyUSB0?registre=1&vitesse=19200[&…]
 * Clés :
 *   registre   obligatoire, numéroté à partir de 1 (comme mbpoll)
 *   type       maintien (défaut, « holding », code 3), entree (« input »,
 *              code 4), bobine (« coil », code 1), discret (code 2)
 *   esclave    adresse de l'esclave (défaut 1, de 0 à 255 ; 1 à 247 en RTU)
 *   nombre     nombre de valeurs (défaut 1, de 1 à 125)
 *   format     entier (16 bits, défaut), signe (16 bits signé), entier32,
 *              signe32, flottant (32 bits IEEE) — les trois derniers prennent
 *              deux registres par valeur
 *   ordre      ordre des mots d'une valeur 32 bits : abcd (défaut, gros
 *              boutiste), cdab (mots inversés), badc, dcba
 *   diviseur   la valeur lue est divisée par ce nombre (défaut 1)
 *   delai      délai de réponse en ms (défaut 1500, de 10 à 60000)
 *   cache      fraîcheur du cache commun en ms (défaut 900 ; 0 = sans cache,
 *              lecture directe) : voir commun.h
 *   perime     au-delà de cette ancienneté (ms, défaut 10000), une valeur
 *              gardée n'est plus rendue ; en deçà, elle l'est tout de suite et
 *              relue en fond (jamais de fenêtre figée, commun.h)
 *   vitesse, parite (N, E, O), bits (7 ou 8), arret (1 ou 2) : RTU seulement
 *              (défauts 19200, E, 8, 1, comme mbpoll)
 * Sortie : une valeur par ligne ; code 0. Échec : rien sur la sortie standard,
 * la raison sur la sortie d'erreur, code 1. Après un échec de connexion, la
 * même base répond sans réseau pendant 5 s (commun.h).
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <modbus.h>

#include <locale.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commun.h"

#define PROGRAMME "sermo-source-modbus"

typedef enum { MAINTIEN, ENTREE, BOBINE, DISCRET } type_t;
typedef enum { ENTIER, SIGNE, ENTIER32, SIGNE32, FLOTTANT } format_t;

static int
choisir(const char *v, const char *const *noms, const int *valeurs, int defaut, int *sortie)
{
	if (!v) {
		*sortie = defaut;
		return 0;
	}
	for (size_t i = 0; noms[i]; i++)
		if (strcmp(v, noms[i]) == 0) {
			*sortie = valeurs[i];
			return 0;
		}
	return -1;
}

/* Valeur 32 bits à partir de deux registres, selon l'ordre des octets. */
static uint32_t
assembler(uint16_t r0, uint16_t r1, const char *ordre)
{
	uint8_t a = r0 >> 8, b = r0 & 0xff, c = r1 >> 8, d = r1 & 0xff;
	if (strcmp(ordre, "cdab") == 0) return (uint32_t)c << 24 | (uint32_t)d << 16 | (uint32_t)a << 8 | b;
	if (strcmp(ordre, "badc") == 0) return (uint32_t)b << 24 | (uint32_t)a << 16 | (uint32_t)d << 8 | c;
	if (strcmp(ordre, "dcba") == 0) return (uint32_t)d << 24 | (uint32_t)c << 16 | (uint32_t)b << 8 | a;
	return (uint32_t)a << 24 | (uint32_t)b << 16 | (uint32_t)c << 8 | d;
}

static void
ecrire(FILE *sortie, double v, double diviseur, int entier)
{
	if (diviseur == 1 && entier)
		fprintf(sortie, "%.0f\n", v);
	else
		fprintf(sortie, "%.15g\n", v / diviseur);
}

/* tcp://hôte[:port] : hôte (nom, IPv4 ou [IPv6]) et port, découpés dans @base. */
static int
tcp_hote_port(char *base, char **hote, char **port)
{
	char *h = base + 6;
	*port = "502";
	if (*h == '[') {
		char *f = strchr(h, ']');
		if (!f || (f[1] != '\0' && f[1] != ':'))
			return -1;
		*f = '\0';
		if (f[1] == ':')
			*port = f + 2;
		*hote = h + 1;
	} else {
		char *deux = strrchr(h, ':');
		if (deux) {
			*deux = '\0';
			*port = deux + 1;
		}
		*hote = h;
	}
	if (**hote == '\0' || strchr(*hote, '/'))
		return -1;
	char *fin;
	long p = strtol(*port, &fin, 10);
	return (*fin || p < 1 || p > 65535) ? -1 : 0;
}

/* Ce que la lecture réseau a besoin de savoir (voir lire_modbus). */
typedef struct {
	modbus_t *ctx;
	const char *base, *ordre;
	int type, format, mots;
	long registre, nombre;
	double diviseur;
} lecture_modbus_t;

/* Le lecteur de source_lire() (commun.h) : connexion, lecture, valeurs. */
static lecture_t
lire_modbus(void *donnees, FILE *sortie)
{
	lecture_modbus_t *l = donnees;
	uint16_t regs[125 * 2];
	uint8_t etats[125];

	if (modbus_connect(l->ctx) != 0) {
		fprintf(stderr, PROGRAMME ": connexion à %s impossible (%s)\n", l->base, modbus_strerror(errno));
		return LECTURE_PANNE;
	}
	int adr = (int)(l->registre - 1), n = (int)(l->nombre * l->mots), lus;
	switch (l->type) {
	case MAINTIEN: lus = modbus_read_registers(l->ctx, adr, n, regs); break;
	case ENTREE:   lus = modbus_read_input_registers(l->ctx, adr, n, regs); break;
	case BOBINE:   lus = modbus_read_bits(l->ctx, adr, n, etats); break;
	default:       lus = modbus_read_input_bits(l->ctx, adr, n, etats); break;
	}
	int erreur = errno;
	modbus_close(l->ctx);
	if (lus != n) {
		fprintf(stderr, PROGRAMME ": lecture refusée ou incomplète (%s)\n", lus < 0 ? modbus_strerror(erreur) : "réponse courte");
		/* Un refus de l'automate (registre absent) n'est pas une panne réseau :
		 * pas de pause, sauf délai dépassé. */
		return lus < 0 && erreur == ETIMEDOUT ? LECTURE_PANNE : LECTURE_REFUS;
	}
	for (int i = 0; i < l->nombre; i++) {
		if (l->type == BOBINE || l->type == DISCRET) {
			fprintf(sortie, "%d\n", etats[i] ? 1 : 0);
			continue;
		}
		switch (l->format) {
		case ENTIER:  ecrire(sortie, regs[i], l->diviseur, 1); break;
		case SIGNE:   ecrire(sortie, (int16_t)regs[i], l->diviseur, 1); break;
		default: {
			uint32_t v = assembler(regs[2 * i], regs[2 * i + 1], l->ordre ? l->ordre : "abcd");
			if (l->format == ENTIER32) {
				ecrire(sortie, v, l->diviseur, 1);
			} else if (l->format == SIGNE32) {
				ecrire(sortie, (int32_t)v, l->diviseur, 1);
			} else {
				float f;
				memcpy(&f, &v, sizeof f);
				/* Un flottant 32 bits garde ~7 chiffres : pas 23.3999996. */
				fprintf(sortie, "%.7g\n", (double)f / l->diviseur);
			}
		}
		}
	}
	return LECTURE_OK;
}

int
main(int argc, char **argv)
{
	static const char *const connues[] = { "registre", "type", "esclave", "nombre", "format", "ordre",
	                                       "diviseur", "delai", "cache", "perime", "vitesse", "parite", "bits", "arret", NULL };
	static const char *const noms_type[] = { "maintien", "holding", "entree", "input", "bobine", "coil", "discret", "discrete", NULL };
	static const int vals_type[] = { MAINTIEN, MAINTIEN, ENTREE, ENTREE, BOBINE, BOBINE, DISCRET, DISCRET };
	static const char *const noms_format[] = { "entier", "signe", "entier32", "signe32", "flottant", NULL };
	static const int vals_format[] = { ENTIER, SIGNE, ENTIER32, SIGNE32, FLOTTANT };
	adresse_t a;

	if (argc != 2) {
		fputs("usage : " PROGRAMME " ADRESSE   (appelé par <input modbus>)\n", stderr);
		return 1;
	}
	if (adresse_lire(PROGRAMME, argv[1], &a) != 0)
		return 1;

	int code = 1, type, format, rtu = strncmp(a.base, "rtu://", 6) == 0;
	long registre, esclave, nombre, delai, cache, perime, vitesse, bits, arret;
	double diviseur = 1;
	const char *ordre = adresse_valeur(&a, "ordre");
	const char *parite = adresse_valeur(&a, "parite");
	modbus_t *ctx = NULL;
	char base[4096];

	snprintf(base, sizeof base, "%s", a.base);
	if (adresse_cles_connues(PROGRAMME, &a, connues) != 0)
		goto fin;
	if (!adresse_valeur(&a, "registre")) {
		fputs(PROGRAMME ": registre manquant (exemple : registre=1)\n", stderr);
		goto fin;
	}
	if (adresse_entier(PROGRAMME, &a, "registre", 1, 65536, 1, &registre)
	    || adresse_entier(PROGRAMME, &a, "esclave", 0, 255, 1, &esclave)
	    || adresse_entier(PROGRAMME, &a, "nombre", 1, 125, 1, &nombre)
	    || adresse_entier(PROGRAMME, &a, "delai", 10, 60000, 1500, &delai)
	    || adresse_entier(PROGRAMME, &a, "cache", 0, 60000, CACHE_DEFAUT_MS, &cache)
	    || adresse_entier(PROGRAMME, &a, "perime", 0, 600000, PERIME_DEFAUT_MS, &perime)
	    || adresse_entier(PROGRAMME, &a, "vitesse", 50, 4000000, 19200, &vitesse)
	    || adresse_entier(PROGRAMME, &a, "bits", 7, 8, 8, &bits)
	    || adresse_entier(PROGRAMME, &a, "arret", 1, 2, 1, &arret))
		goto fin;
	if (choisir(adresse_valeur(&a, "type"), noms_type, vals_type, MAINTIEN, &type) != 0) {
		fputs(PROGRAMME ": type vaut maintien, entree, bobine ou discret\n", stderr);
		goto fin;
	}
	if (choisir(adresse_valeur(&a, "format"), noms_format, vals_format, ENTIER, &format) != 0) {
		fputs(PROGRAMME ": format vaut entier, signe, entier32, signe32 ou flottant\n", stderr);
		goto fin;
	}
	if (ordre && strcmp(ordre, "abcd") && strcmp(ordre, "cdab") && strcmp(ordre, "badc") && strcmp(ordre, "dcba")) {
		fputs(PROGRAMME ": ordre vaut abcd, cdab, badc ou dcba\n", stderr);
		goto fin;
	}
	if (parite && strcmp(parite, "N") && strcmp(parite, "E") && strcmp(parite, "O")) {
		fputs(PROGRAMME ": parite vaut N, E ou O\n", stderr);
		goto fin;
	}
	if (adresse_valeur(&a, "diviseur")) {
		char *f;
		const char *v = adresse_valeur(&a, "diviseur");
		/* Le diviseur s'écrit avec un point, quelle que soit la langue :
		 * strtod_l en locale C (strtod suivrait LC_NUMERIC). */
		locale_t c = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
		errno = 0;
		diviseur = c ? strtod_l(v, &f, c) : 0; /* garde:primitive-locale */
		if (c)
			freelocale(c);
		if (errno || f == v || *f || !isfinite(diviseur) || diviseur <= 0) {
			fprintf(stderr, PROGRAMME ": diviseur doit être un nombre positif (lu : %s)\n", v);
			goto fin;
		}
	}
	int mots = (format == ENTIER32 || format == SIGNE32 || format == FLOTTANT) ? 2 : 1;
	if ((type == BOBINE || type == DISCRET) && (format != ENTIER || ordre)) {
		fputs(PROGRAMME ": format et ordre ne s'appliquent pas aux bobines ni aux entrées discrètes\n", stderr);
		goto fin;
	}
	if (registre - 1 + nombre * mots > 65536) {
		fputs(PROGRAMME ": la lecture dépasse le registre 65536\n", stderr);
		goto fin;
	}

	if (rtu) {
		const char *port = a.base + 6;
		if (port[0] != '/' || (esclave < 1 || esclave > 247)) {
			fputs(PROGRAMME ": RTU : rtu:///dev/… et esclave de 1 à 247\n", stderr);
			goto fin;
		}
		ctx = modbus_new_rtu(port, (int)vitesse, parite ? parite[0] : 'E', (int)bits, (int)arret);
	} else if (strncmp(a.base, "tcp://", 6) == 0) {
		char *hote, *port;
		if (tcp_hote_port(a.base, &hote, &port) != 0) {
			fputs(PROGRAMME ": adresse TCP attendue : tcp://hôte[:port]\n", stderr);
			goto fin;
		}
		ctx = modbus_new_tcp_pi(hote, port);
	} else {
		fputs(PROGRAMME ": l'adresse commence par tcp:// ou rtu://\n", stderr);
		goto fin;
	}
	if (!ctx) {
		fprintf(stderr, PROGRAMME ": %s\n", modbus_strerror(errno));
		goto fin;
	}
	modbus_set_slave(ctx, (int)esclave);
	modbus_set_response_timeout(ctx, (uint32_t)(delai / 1000), (uint32_t)(delai % 1000) * 1000);
	lecture_modbus_t lecture = { ctx, base, ordre, type, format, mots, registre, nombre, diviseur };
	code = source_lire(PROGRAMME, "modbus", argv[1], base, cache, perime, lire_modbus, &lecture);
fin:
	if (ctx) {
		modbus_close(ctx);
		modbus_free(ctx);
	}
	adresse_liberer(&a);
	return code;
}
