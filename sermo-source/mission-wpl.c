/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mission-wpl.c — lecture d'une mission « QGC WPL 110 ». Contrat : mission-wpl.h.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mission-wpl.h"

#define LIGNE_MAX 512
#define CHAMPS 12

static int __attribute__((format(printf, 3, 4)))
refus(char *raison, size_t taille, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vsnprintf(raison, taille, format, ap);
	va_end(ap);
	return -1;
}

/* Nombre écrit avec un point, en entier ; NAN sinon. */
static double
nombre(const char *texte, locale_t c)
{
	char *fin;
	errno = 0;
	double v = strtod_l(texte, &fin, c); /* garde:primitive-locale */
	if (errno || fin == texte || *fin || !isfinite(v))
		return NAN;
	return v;
}

int
mission_lire(const char *texte, mission_t *m, char *raison, size_t taille)
{
	m->n = 0;
	if (strlen(texte) > MISSION_OCTETS_MAX)
		return refus(raison, taille, "fichier de mission trop long (256 Kio au plus)");
	locale_t c = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
	if (!c)
		return refus(raison, taille, "mémoire épuisée");

	int numero = 0, rang_attendu = 0, r = -1;
	const char *p = texte;
	while (*p) {
		const char *fin = strchr(p, '\n');
		size_t n = fin ? (size_t)(fin - p) : strlen(p);
		numero++;
		if (n >= LIGNE_MAX) {
			refus(raison, taille, "ligne %d trop longue", numero);
			goto sortie;
		}
		char ligne[LIGNE_MAX];
		memcpy(ligne, p, n);
		ligne[n] = '\0';
		p = fin ? fin + 1 : p + n;
		if (n && ligne[n - 1] == '\r')
			ligne[n - 1] = '\0';
		for (char *q = ligne; *q; q++)
			if ((unsigned char)*q < 0x20 && *q != '\t') {
				refus(raison, taille, "ligne %d : caractère de contrôle", numero);
				goto sortie;
			}
		if (numero == 1) {
			if (strcmp(ligne, "QGC WPL 110") != 0) {
				refus(raison, taille, "ce n'est pas un fichier « QGC WPL 110 » (première ligne)");
				goto sortie;
			}
			continue;
		}
		char *champ[CHAMPS + 1], *s2 = NULL;
		int k = 0;
		for (char *t = strtok_r(ligne, " \t", &s2); t && k <= CHAMPS; t = strtok_r(NULL, " \t", &s2))
			champ[k++] = t;
		if (k == 0)
			continue;
		if (k != CHAMPS) {
			refus(raison, taille, "ligne %d : %d champs au lieu de 12", numero, k);
			goto sortie;
		}
		double v[CHAMPS];
		for (int i = 0; i < CHAMPS; i++) {
			v[i] = nombre(champ[i], c);
			if (isnan(v[i])) {
				refus(raison, taille, "ligne %d : champ %d « %s » n'est pas un nombre", numero, i + 1, champ[i]);
				goto sortie;
			}
		}
		if (v[0] != rang_attendu) {
			refus(raison, taille, "ligne %d : rang %g au lieu de %d (les rangs se suivent depuis 0)",
			      numero, v[0], rang_attendu);
			goto sortie;
		}
		rang_attendu++;
		if (v[0] == 0)
			continue;   /* le « départ » : ArduPilot fixe le sien */
		if (m->n >= MISSION_POINTS_MAX) {
			refus(raison, taille, "plus de %d points", MISSION_POINTS_MAX);
			goto sortie;
		}
		if (v[3] != 16) {
			refus(raison, taille, "ligne %d : commande %g refusée (seul le point de passage, 16, est permis)",
			      numero, v[3]);
			goto sortie;
		}
		if (v[2] != 3) {
			refus(raison, taille, "ligne %d : repère %g refusé (seul 3, hauteur au-dessus du départ, est permis)",
			      numero, v[2]);
			goto sortie;
		}
		if (v[4] < 0 || v[4] > MISSION_ATTENTE_MAX || v[5] != 0 || v[6] != 0 || v[7] != 0) {
			refus(raison, taille, "ligne %d : seule l'attente au point (p1, de 0 à 60 s) est permise ; p2 à p4 "
			      "valent 0", numero);
			goto sortie;
		}
		if (v[11] != 1) {
			refus(raison, taille, "ligne %d : « continuer » doit valoir 1", numero);
			goto sortie;
		}
		if (v[8] < -90 || v[8] > 90 || v[9] < -180 || v[9] > 180 || (v[8] == 0 && v[9] == 0)) {
			refus(raison, taille, "ligne %d : position impossible (%g, %g)", numero, v[8], v[9]);
			goto sortie;
		}
		point_t *pt = &m->point[m->n++];
		pt->latitude = v[8];
		pt->longitude = v[9];
		pt->hauteur = v[10];
		pt->attente = v[4];
	}
	if (numero == 0) {
		refus(raison, taille, "fichier de mission vide");
		goto sortie;
	}
	if (m->n == 0) {
		refus(raison, taille, "aucun point de passage dans la mission");
		goto sortie;
	}
	r = 0;
sortie:
	freelocale(c);
	if (r)
		m->n = 0;
	return r;
}

double
mission_distance(double lat1, double lon1, double lat2, double lon2)
{
	const double R = 6371000.0, rad = M_PI / 180.0;
	double dlat = (lat2 - lat1) * rad, dlon = (lon2 - lon1) * rad;
	double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1 * rad) * cos(lat2 * rad) * sin(dlon / 2) * sin(dlon / 2);
	return 2 * R * atan2(sqrt(a), sqrt(1 - a));
}
