/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * test_sermo_source.c — Tests de COMPORTEMENT de la traduction de
 * <input modbus> / <input opcua> / <input mqtt> / <input mavlink>, <action mqtt> /
 * <action mavlink> (libsermocore/include/sermo_source.h).
 * haplo-dialog — cœur partagé (GPL-2.0-or-later)
 *
 * Vérifie, contre safe_exec.c lui-même, que la commande produite n'a AUCUN
 * métacaractère (elle part en argv, jamais par /bin/sh), que l'encodage est
 * réversible, et que les entrées douteuses sont refusées.
 * « main() pur » : ni libcheck ni serveur X requis.
 * Sortie : exit 0 si tout passe, 1 sinon.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "sermo_source.h"

static int failures = 0;

#define EXPECT(cond, msg)                                              \
    do {                                                               \
        if (!(cond)) {                                                 \
            fprintf(stderr, "FAIL  %s\n", (msg)); failures++;          \
        } else {                                                       \
            printf("PASS  %s\n", (msg));                               \
        }                                                              \
    } while (0)

/* La liste de safe_exec.c (SHELL_METACHARACTERS), plus l'espace et la fin de
 * ligne qui couperaient l'argument. */
#define INTERDITS "|&;<>(){}$`\\\"'~*? \t\n%"

static char *
traduire(const char *protocole, const char *adresse, char *erreur)
{
	return sermo_source_commande(protocole, adresse, erreur, 160);
}

/* Décode %XX : l'inverse de l'encodage, pour vérifier qu'il est réversible. */
static char *
decoder(const char *s)
{
	char *r = g_malloc(strlen(s) + 1), *w = r;
	for (; *s; s++) {
		if (*s == '%' && s[1] && s[2]) {
			char h[3] = { s[1], s[2], 0 };
			*w++ = (char)strtol(h, NULL, 16);
			s += 2;
		} else {
			*w++ = *s;
		}
	}
	*w = 0;
	return r;
}

int
main(void)
{
	char erreur[160];
	char *c;

	g_unsetenv("SERMO_SOURCE_DIR");
	const char *adresses[] = {
		"tcp://127.0.0.1:5020?esclave=1&registre=1&diviseur=10",
		"opc.tcp://hôte:4840?noeud=ns=1;s=ma variable&mode=chiffre",
		"tcp://h?registre=1&x=$(id)`id`;rm -rf ~ \"'|<>*?%41",
		NULL
	};
	for (int i = 0; adresses[i]; i++) {
		c = traduire(i == 1 ? "opcua" : "modbus", adresses[i], erreur);
		EXPECT(c != NULL, "adresse acceptée");
		if (!c)
			continue;
		const char *prefixe = i == 1 ? "/usr/libexec/sermo/sermo-source-opcua " : "/usr/libexec/sermo/sermo-source-modbus ";
		EXPECT(strncmp(c, prefixe, strlen(prefixe)) == 0, "programme d'aide du dossier par défaut");
		const char *arg = c + strlen(prefixe);
		/* Le « % » de l'encodage lui-même est admis, rien d'autre. */
		gboolean propre = TRUE;
		for (const char *p = c; *p; p++)
			if (*p != '%' && strchr(INTERDITS, *p) && !(p == arg - 1 && *p == ' '))
				propre = FALSE;
		EXPECT(propre, "aucun métacaractère dans la commande");
		EXPECT(strchr(arg, ' ') == NULL, "l'adresse tient en UN argument");
		gchar **argv = NULL;
		EXPECT(g_shell_parse_argv(c, NULL, &argv, NULL) && g_strv_length(argv) == 2,
		       "g_shell_parse_argv y voit deux mots");
		char *rendu = decoder(arg);
		EXPECT(argv && strcmp(argv[1], arg) == 0 && strcmp(rendu, adresses[i]) == 0,
		       "encodage réversible : l'adresse revient telle qu'écrite");
		g_free(rendu);
		g_strfreev(argv);
		free(c);
	}

	free(traduire("modbus", "tcp://h?registre=1", erreur));
	/* <input mqtt> : même traduction ; « / » du sujet tel quel, « # » encodé. */
	c = traduire("mqtt", "mqtt://h:1883?sujet=maison/salon/temperature", erreur);
	EXPECT(c && strcmp(c, "/usr/libexec/sermo/sermo-source-mqtt mqtt://h:1883%3Fsujet=maison/salon/temperature") == 0,
	       "mqtt traduit vers sermo-source-mqtt");
	free(c);
	c = traduire("mqtt", "mqtt://h?sujet=a/#;rm -rf ~", erreur);
	EXPECT(c && !strpbrk(c + strlen("/usr/libexec/sermo/sermo-source-mqtt "), " ;#~"),
	       "mqtt : joker et métacaractères encodés (le refus du joker est au programme d'aide)");
	free(c);
	/* <input mavlink> : lecture de télémétrie ; jamais d'écriture mavlink ici. */
	c = traduire("mavlink", "udp://:14550?champ=tension", erreur);
	EXPECT(c && strcmp(c, "/usr/libexec/sermo/sermo-source-mavlink udp://:14550%3Fchamp=tension") == 0,
	       "mavlink traduit vers sermo-source-mavlink");
	free(c);
	/* <action mavlink> : vers le programme d'ÉCRITURE à part (sermo-cible-mavlink),
	 * jamais vers le lecteur. */
	c = sermo_cible_commande("mavlink", "tcp://h:5760?ordre=rtl", erreur, sizeof erreur);
	EXPECT(c && strcmp(c, "/usr/libexec/sermo/sermo-cible-mavlink tcp://h:5760%3Fordre=rtl") == 0,
	       "action mavlink traduite vers sermo-cible-mavlink");
	free(c);
	c = sermo_cible_commande("mavlink", "tcp://h:5760?parametre=WP_SPD&valeur=$V;id", erreur, sizeof erreur);
	EXPECT(c && !strpbrk(c + strlen("/usr/libexec/sermo/sermo-cible-mavlink "), " ;$&"),
	       "action mavlink : $, & et ; encodés");
	free(c);
	/* <input media> / <action media> : lecteur son/vidéo ; la commande
	 * part vers sermo-cible-media, l'état se lit par sermo-source-media. */
	c = traduire("media", "media://salon?donnee=etat", erreur);
	EXPECT(c && strcmp(c, "/usr/libexec/sermo/sermo-source-media media://salon%3Fdonnee=etat") == 0,
	       "media traduit vers sermo-source-media");
	free(c);
	c = sermo_cible_commande("media", "media://salon?commande=lire&source=file:///a b.ogg;id", erreur, sizeof erreur);
	EXPECT(c && strncmp(c, "/usr/libexec/sermo/sermo-cible-media media://salon%3Fcommande=lire%26source=", 75) == 0
	       && !strpbrk(c + strlen("/usr/libexec/sermo/sermo-cible-media "), " ;&?"),
	       "action media traduite vers sermo-cible-media, espace ; & ? encodés");
	free(c);
	/* <action mqtt> : vers le programme d'ÉCRITURE ; mqtt, mavlink et media seulement. */
	c = sermo_cible_commande("mqtt", "mqtt://h?sujet=a/b&valeur=on", erreur, sizeof erreur);
	EXPECT(c && strcmp(c, "/usr/libexec/sermo/sermo-cible-mqtt mqtt://h%3Fsujet=a/b%26valeur=on") == 0,
	       "action mqtt traduite vers sermo-cible-mqtt");
	free(c);
	c = sermo_cible_commande("modbus", "tcp://h?registre=1", erreur, sizeof erreur);
	EXPECT(c == NULL && strstr(erreur, "mqtt"), "écriture modbus refusée (industrie en lecture seule)");
	c = sermo_cible_commande("opcua", "opc.tcp://h", erreur, sizeof erreur);
	EXPECT(c == NULL, "écriture opcua refusée");
	c = sermo_cible_commande("mqtt", "mqtt://h?sujet=a\nb", erreur, sizeof erreur);
	EXPECT(c == NULL && strstr(erreur, "contrôle"), "action mqtt : fin de ligne refusée");
	c = traduire("http", "x", erreur);
	EXPECT(c == NULL && strstr(erreur, "protocole"), "protocole inconnu refusé");
	c = traduire("modbus", "", erreur);
	EXPECT(c == NULL && strstr(erreur, "vide"), "adresse vide refusée");
	c = traduire("modbus", "tcp://h\n?registre=1", erreur);
	EXPECT(c == NULL && strstr(erreur, "contrôle"), "fin de ligne refusée");
	char *longue = g_malloc(SERMO_SOURCE_ADRESSE_MAX + 2);
	memset(longue, 'a', SERMO_SOURCE_ADRESSE_MAX + 1);
	longue[SERMO_SOURCE_ADRESSE_MAX + 1] = 0;
	c = traduire("modbus", longue, erreur);
	EXPECT(c == NULL && strstr(erreur, "4096"), "adresse de 4097 octets refusée");
	longue[SERMO_SOURCE_ADRESSE_MAX] = 0;
	c = traduire("modbus", longue, erreur);
	EXPECT(c != NULL, "adresse de 4096 octets acceptée (borne)");
	free(c);
	g_free(longue);

	g_setenv("SERMO_SOURCE_DIR", "/opt/aide", TRUE);
	c = traduire("opcua", "opc.tcp://h", erreur);
	EXPECT(c && strcmp(c, "/opt/aide/sermo-source-opcua opc.tcp://h") == 0, "SERMO_SOURCE_DIR suivi");
	free(c);
	g_setenv("SERMO_SOURCE_DIR", "relatif", TRUE);
	EXPECT(traduire("opcua", "x", erreur) == NULL, "SERMO_SOURCE_DIR relatif refusé");
	g_setenv("SERMO_SOURCE_DIR", "/a b", TRUE);
	EXPECT(traduire("opcua", "x", erreur) == NULL, "SERMO_SOURCE_DIR avec espace refusé");
	g_setenv("SERMO_SOURCE_DIR", "/a;id", TRUE);
	EXPECT(traduire("opcua", "x", erreur) == NULL, "SERMO_SOURCE_DIR avec « ; » refusé");

	printf("%s : %d échec(s)\n", failures ? "ÉCHEC" : "OK", failures);
	return failures ? 1 : 0;
}
