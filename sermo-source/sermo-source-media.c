/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo-source-media — programme d'aide de <input media> : l'état
 * d'un lecteur son/vidéo lancé par sermo-cible-media. Lecture seule.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 *   media://NOM?donnee=etat       stopped | playing | paused | error   (défaut)
 *   media://NOM?donnee=position   secondes écoulées (« 12.3 »), vide si arrêté
 *   media://NOM?donnee=duree      durée totale en secondes, vide si inconnue
 *
 * « etat » est la valeur exportée du cahier (D-11) : déterministe, toujours
 * un de ces quatre mots ; « error » quand mpv s'est arrêté en échec.
 * Code de sortie : 0 (une valeur, éventuellement vide, a été écrite) ; 1 si
 * l'adresse est refusée.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commun.h"
#include "media-commun.h"

#define PROGRAMME "sermo-source-media"

/* La valeur après « "data": » (espaces tolérés) dans une réponse de mpv. */
static const char *
donnee_de(const char *rep)
{
	const char *p = strstr(rep, "\"data\":");
	if (!p)
		return NULL;
	p += 7;
	while (*p == ' ')
		p++;
	return p;
}

/* Un nombre après « "data": ». */
static int
nombre(const char *rep, double *v)
{
	const char *p = donnee_de(rep);
	if (!p)
		return -1;
	/* Le JSON de mpv : point décimal quelle que soit la locale du poste. */
	char *fin;
	locale_t lc = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
	if (!lc)
		return -1;
	*v = strtod_l(p, &fin, lc); /* garde:primitive-locale */
	freelocale(lc);
	return fin == p ? -1 : 0;
}

int
main(int argc, char **argv)
{
	adresse_t a;
	char nom[MEDIA_NOM_MAX + 1], dossier[600], rep[1024];
	static const char *const connues[] = { "donnee", NULL };
	if (argc != 2) {
		fputs("usage : " PROGRAMME " 'media://NOM?donnee=etat'\n", stderr);
		return 1;
	}
	if (adresse_lire(PROGRAMME, argv[1], &a) != 0)
		return 1;
	int rc = 1;
	if (adresse_cles_connues(PROGRAMME, &a, connues) != 0 || media_nom(PROGRAMME, a.base, nom, sizeof nom) != 0)
		goto fin;
	const char *donnee = adresse_valeur(&a, "donnee");
	if (!donnee || !*donnee)
		donnee = "etat";
	int etat = strcmp(donnee, "etat") == 0, pos = strcmp(donnee, "position") == 0, duree = strcmp(donnee, "duree") == 0;
	if (!etat && !pos && !duree) {
		fprintf(stderr, PROGRAMME ": donnee=%s inconnue (etat, position, duree)\n", donnee);
		goto fin;
	}
	rc = 0;
	int present = media_dossier(PROGRAMME, nom, dossier, sizeof dossier, 0) == 0;
	int vivant = present && media_pid_vivant(dossier, "gardien");
	if (!vivant) {
		if (etat) {
			char mot[16] = "";
			char chemin[620];
			snprintf(chemin, sizeof chemin, "%s/fin", dossier);
			FILE *f = present ? fopen(chemin, "r") : NULL;
			if (f) { if (!fgets(mot, sizeof mot, f)) mot[0] = 0; fclose(f); }
			puts(strncmp(mot, "erreur", 6) == 0 ? "error" : "stopped");
		} else {
			puts("");
		}
		goto fin;
	}
	if (etat) {
		if (media_ipc(dossier, "{\"command\":[\"get_property\",\"pause\"]}", rep, sizeof rep) != 0)
			puts("playing");               /* mpv démarre : le socket n'est pas encore prêt */
		else
		{
			const char *d = donnee_de(rep);
			puts(d && strncmp(d, "true", 4) == 0 ? "paused" : "playing");
		}
		goto fin;
	}
	double v;
	const char *req = pos ? "{\"command\":[\"get_property\",\"time-pos\"]}"
	                      : "{\"command\":[\"get_property\",\"duration\"]}";
	if (media_ipc(dossier, req, rep, sizeof rep) == 0 && nombre(rep, &v) == 0 && v >= 0 && v < 1e7)
		printf("%.1f\n", v);
	else
		puts("");
fin:
	adresse_liberer(&a);
	return rc;
}
