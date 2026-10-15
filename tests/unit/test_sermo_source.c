/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * test_sermo_source.c — Tests de COMPORTEMENT de la traduction de
 * <input modbus> / <input opcua> (libsermocore/include/sermo_source.h).
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
