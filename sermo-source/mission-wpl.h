/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mission-wpl.h — lecture d'une mission « QGC WPL 110 » (Mission Planner,
 * QGroundControl) pour sermo-cible-mavlink, réduite aux points de passage.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Format : première ligne « QGC WPL 110 », puis une ligne par élément, 12
 * champs séparés par des tabulations (ou des espaces) :
 *   rang courant repère commande p1 p2 p3 p4 latitude longitude hauteur continuer
 * Le rang 0 (le « départ » enregistré par ces logiciels) est lu et ignoré.
 * Chaque autre élément doit être un point de passage simple : commande 16
 * (MAV_CMD_NAV_WAYPOINT), repère 3 (hauteur au-dessus du départ), p1 (attente
 * au point) de 0 à 60 s, p2 à p4 nuls, continuer = 1. Tout autre élément
 * refuse le fichier entier.
 */
#ifndef SERMO_MISSION_WPL_H
#define SERMO_MISSION_WPL_H

#include <stddef.h>

#define MISSION_POINTS_MAX 100
#define MISSION_OCTETS_MAX (256 * 1024)
#define MISSION_ATTENTE_MAX 60.0

typedef struct {
	double latitude, longitude;   /* degrés */
	double hauteur;               /* m au-dessus du départ */
	double attente;               /* s au point */
} point_t;

typedef struct {
	point_t point[MISSION_POINTS_MAX];
	int n;
} mission_t;

/* Lit @texte (terminé par NUL). Rend 0, ou -1 avec la raison dans @raison. */
int mission_lire(const char *texte, mission_t *m, char *raison, size_t taille);

/* Distance au sol en mètres entre deux positions (degrés), formule de haversine. */
double mission_distance(double lat1, double lon1, double lat2, double lon2);

#endif
