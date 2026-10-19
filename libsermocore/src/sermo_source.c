/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_source.c — traduction de <input modbus> / <input opcua> / <input mqtt> /
 * <input mavlink> / <input media>, et de <action mqtt> / <action mavlink> /
 * <action media>, en commande.
 * Contrat : include/sermo_source.h.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sermo_source.h"

/* Octets transmis tels quels : aucun n'est un métacaractère pour safe_exec, ni
 * un séparateur pour g_shell_parse_argv. */
static int
octet_sur(unsigned char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
	       || strchr("._:/=,+@-", c) != NULL;
}

static void
dire(char *erreur, size_t taille, const char *message)
{
	if (erreur && taille)
		snprintf(erreur, taille, "%s", message);
}

/* @genre : "source" (lecture) ou "cible" (écriture) ; le programme lancé est
 * <dossier>/sermo-<genre>-<protocole>. */
static char *
traduire(const char *genre, const char *protocole, const char *adresse,
         char *erreur, size_t taille)
{
	if (!adresse || *adresse == '\0') {
		dire(erreur, taille, "l'adresse est vide");
		return NULL;
	}
	size_t n = strlen(adresse);
	if (n > SERMO_SOURCE_ADRESSE_MAX) {
		dire(erreur, taille, "l'adresse dépasse 4096 octets");
		return NULL;
	}
	for (size_t i = 0; i < n; i++) {
		unsigned char c = (unsigned char)adresse[i];
		if (c < 0x20 || c == 0x7f) {
			dire(erreur, taille, "l'adresse contient un caractère de contrôle (fin de ligne, tabulation…)");
			return NULL;
		}
	}

	const char *dossier = getenv("SERMO_SOURCE_DIR");
	if (!dossier || *dossier == '\0')
		dossier = SERMO_SOURCE_DIR_DEFAUT;
	if (dossier[0] != '/') {
		dire(erreur, taille, "SERMO_SOURCE_DIR doit être un chemin absolu");
		return NULL;
	}
	for (const char *p = dossier; *p; p++) {
		if (!octet_sur((unsigned char)*p)) {
			dire(erreur, taille, "SERMO_SOURCE_DIR contient un caractère interdit (espace, guillemet, $…)");
			return NULL;
		}
	}

	/* dossier + "/sermo-" + genre + "-" + protocole + " " + adresse encodée (×3) + NUL */
	size_t total = strlen(dossier) + 7 + strlen(genre) + 1 + strlen(protocole) + 1 + 3 * n + 1;
	char *commande = malloc(total);
	if (!commande) {
		dire(erreur, taille, "mémoire épuisée");
		return NULL;
	}
	int ecrit = snprintf(commande, total, "%s/sermo-%s-%s ", dossier, genre, protocole);
	if (ecrit < 0 || (size_t)ecrit >= total) {
		free(commande);
		dire(erreur, taille, "commande trop longue");
		return NULL;
	}
	char *q = commande + ecrit;
	static const char hex[] = "0123456789ABCDEF";
	for (size_t i = 0; i < n; i++) {
		unsigned char c = (unsigned char)adresse[i];
		if (octet_sur(c)) {
			*q++ = (char)c;
		} else {
			*q++ = '%';
			*q++ = hex[c >> 4];
			*q++ = hex[c & 0x0f];
		}
	}
	*q = '\0';
	return commande;
}

char *
sermo_source_commande(const char *protocole, const char *adresse,
                      char *erreur, size_t taille)
{
	if (!protocole || (strcmp(protocole, "modbus") != 0 && strcmp(protocole, "opcua") != 0
	                   && strcmp(protocole, "mqtt") != 0 && strcmp(protocole, "mavlink") != 0
	                   && strcmp(protocole, "media") != 0)) {
		dire(erreur, taille, "protocole inconnu (modbus, opcua, mqtt, mavlink ou media)");
		return NULL;
	}
	return traduire("source", protocole, adresse, erreur, taille);
}

char *
sermo_cible_commande(const char *protocole, const char *adresse,
                     char *erreur, size_t taille)
{
	/* Écriture : MQTT, MAVLink, et la commande d'un lecteur son/vidéo (media,
	 *). Jamais Modbus ni OPC UA (lecture seule, par construction) ;
	 * ces protocoles n'ont pas de programme d'écriture. */
	if (!protocole || (strcmp(protocole, "mqtt") != 0 && strcmp(protocole, "mavlink") != 0
	                   && strcmp(protocole, "media") != 0)) {
		dire(erreur, taille, "écriture permise vers mqtt, mavlink et media seulement");
		return NULL;
	}
	return traduire("cible", protocole, adresse, erreur, taille);
}
