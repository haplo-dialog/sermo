/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo-source-opcua — programme d'aide de <input opcua> : lit UNE variable
 * d'un serveur OPC UA et écrit sa valeur, seule, sur la sortie standard.
 * Lecture SEULE : aucun service d'écriture ni d'appel de méthode.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Appelé par sermo (voir libsermocore/include/sermo_source.h) avec l'adresse
 * encodée en un argument :
 *   opc.tcp://hôte:port?noeud=ns=1;s=temperature[&clé=valeur…]
 * Clés :
 *   noeud        obligatoire, notation OPC UA (ns=1;s=…, ns=0;i=2258…)
 *   delai        délai de réponse en ms (défaut 1500, de 10 à 60000)
 *   mode         none (défaut), sign ou chiffre
 *   certificat   } fichiers DER ou PEM du poste ; obligatoires avec
 *   cle          } sign et chiffre
 *   confiance    certificat de serveur admis ; répétable (16 au plus),
 *                au moins un avec sign et chiffre. Aucun mode « tout accepter ».
 *   uri          ApplicationUri du certificat (défaut urn:sermo:opcua-lire)
 *   cache        fraîcheur du cache commun en ms (défaut 900 ; 0 = sans cache,
 *                lecture directe) : voir commun.h
 *   perime       au-delà de cette ancienneté (ms, défaut 10000), une valeur
 *                gardée n'est plus rendue ; en deçà, elle l'est tout de suite et
 *                relue en fond (jamais de fenêtre figée, commun.h)
 * Sortie : la valeur et une fin de ligne ; code 0. Échec : rien sur la sortie
 * standard, la raison sur la sortie d'erreur, code 1. Après un échec, la même
 * base répond sans réseau pendant 5 s (commun.h).
 */
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commun.h"

#define PROGRAMME "sermo-source-opcua"
#define CONFIANCE_MAX 16

static void
journal(void *contexte, UA_LogLevel niveau, UA_LogCategory categorie,
        const char *message, va_list arguments)
{
	(void)contexte;
	(void)categorie;
	if (niveau < UA_LOGLEVEL_ERROR)
		return;
	fputs(PROGRAMME ": ", stderr);
	vfprintf(stderr, message, arguments);
	fputc('\n', stderr);
}

static UA_Logger journaliseur = { journal, NULL, NULL };

/* Une chaîne : ses octets, fins de ligne remplacées par une espace. */
static void
ecrire_texte(FILE *f, const UA_Byte *d, size_t n)
{
	for (size_t i = 0; i < n; i++)
		fputc(d[i] == '\n' || d[i] == '\r' ? ' ' : d[i], f);
}

static void
ecrire_valeur(FILE *f, const UA_Variant *v)
{
	if (UA_Variant_isEmpty(v))
		return;
	if (UA_Variant_isScalar(v)) {
		const void *d = v->data;
		const UA_DataType *t = v->type;
		if (t == &UA_TYPES[UA_TYPES_BOOLEAN])  { fputs(*(const UA_Boolean *)d ? "true" : "false", f); return; }
		if (t == &UA_TYPES[UA_TYPES_SBYTE])    { fprintf(f, "%d", *(const UA_SByte *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_BYTE])     { fprintf(f, "%u", *(const UA_Byte *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_INT16])    { fprintf(f, "%d", *(const UA_Int16 *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_UINT16])   { fprintf(f, "%u", *(const UA_UInt16 *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_INT32])    { fprintf(f, "%" PRId32, *(const UA_Int32 *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_UINT32])   { fprintf(f, "%" PRIu32, *(const UA_UInt32 *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_INT64])    { fprintf(f, "%" PRId64, *(const UA_Int64 *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_UINT64])   { fprintf(f, "%" PRIu64, *(const UA_UInt64 *)d); return; }
		/* Les chiffres que le type garde vraiment ; un point (locale C). */
		if (t == &UA_TYPES[UA_TYPES_FLOAT])    { fprintf(f, "%.7g", (double)*(const UA_Float *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_DOUBLE])   { fprintf(f, "%.15g", *(const UA_Double *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_DATETIME]) {
			UA_DateTimeStruct h = UA_DateTime_toStruct(*(const UA_DateTime *)d);
			fprintf(f, "%04d-%02u-%02uT%02u:%02u:%02u.%03uZ", h.year, h.month, h.day,
			       h.hour, h.min, h.sec, h.milliSec);
			return;
		}
		if (t == &UA_TYPES[UA_TYPES_STRING]) {
			const UA_String *s = d;
			ecrire_texte(f, s->data, s->length);
			return;
		}
		if (t == &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]) {
			const UA_LocalizedText *s = d;
			ecrire_texte(f, s->text.data, s->text.length);
			return;
		}
	}
	UA_String texte = UA_STRING_NULL;
	if (UA_print(v, &UA_TYPES[UA_TYPES_VARIANT], &texte) == UA_STATUSCODE_GOOD)
		ecrire_texte(f, texte.data, texte.length);
	UA_String_clear(&texte);
}

/* Lit un fichier entier (certificat, clé) ; 1 Mio au plus. */
static int
charger(const char *chemin, UA_ByteString *sortie)
{
	UA_ByteString_init(sortie);
	FILE *f = fopen(chemin, "rb");
	if (!f) {
		fprintf(stderr, PROGRAMME ": %s : %s\n", chemin, strerror(errno));
		return -1;
	}
	if (UA_ByteString_allocBuffer(sortie, 1 << 20) != UA_STATUSCODE_GOOD) {
		fclose(f);
		return -1;
	}
	size_t n = fread(sortie->data, 1, 1 << 20, f);
	int erreur = ferror(f);
	fclose(f);
	if (erreur || n == 0 || n == (1 << 20)) {
		fprintf(stderr, PROGRAMME ": %s : fichier illisible, vide ou trop grand\n", chemin);
		memset(sortie->data, 0, 1 << 20);
		UA_ByteString_clear(sortie);
		return -1;
	}
	sortie->length = n;
	return 0;
}

static void
effacer(UA_ByteString *b)
{
	if (b->data)
		memset(b->data, 0, b->length);
	UA_ByteString_clear(b);
}

/* Configure la sécurité ; rend 0 ou -1 (message déjà écrit). */
static int
securiser(UA_ClientConfig *config, const adresse_t *a, UA_MessageSecurityMode mode)
{
	const char *certificat = adresse_valeur(a, "certificat");
	const char *cle = adresse_valeur(a, "cle");
	const char *uri = adresse_valeur(a, "uri");
	const char *confiance[CONFIANCE_MAX];
	size_t nconfiance = 0;
	for (size_t i = 0; i < a->n; i++) {
		if (strcmp(a->cle[i], "confiance") != 0)
			continue;
		if (nconfiance == CONFIANCE_MAX) {
			fputs(PROGRAMME ": 16 certificats de confiance au plus\n", stderr);
			return -1;
		}
		confiance[nconfiance++] = a->valeur[i];
	}
	if (mode == UA_MESSAGESECURITYMODE_NONE) {
		if (certificat || cle || nconfiance || uri) {
			fputs(PROGRAMME ": certificat, cle, confiance et uri demandent mode=sign ou mode=chiffre\n", stderr);
			return -1;
		}
		return UA_ClientConfig_setDefault(config) == UA_STATUSCODE_GOOD ? 0 : -1;
	}
	if (!certificat || !cle || nconfiance == 0) {
		fputs(PROGRAMME ": mode=sign et mode=chiffre demandent certificat, cle et au moins une confiance\n", stderr);
		return -1;
	}
	UA_ByteString cert, clef, liste[CONFIANCE_MAX];
	for (size_t j = 0; j < nconfiance; j++)
		UA_ByteString_init(&liste[j]);
	UA_ByteString_init(&clef);
	int ok = charger(certificat, &cert) == 0 && charger(cle, &clef) == 0;
	for (size_t j = 0; ok && j < nconfiance; j++)
		ok = charger(confiance[j], &liste[j]) == 0;
	UA_StatusCode rc = ok ? UA_ClientConfig_setDefaultEncryption(config, cert, clef, liste, nconfiance, NULL, 0)
	                      : UA_STATUSCODE_BADINTERNALERROR;
	UA_ByteString_clear(&cert);
	effacer(&clef);
	for (size_t j = 0; j < nconfiance; j++)
		UA_ByteString_clear(&liste[j]);
	if (!ok)
		return -1;
	if (rc != UA_STATUSCODE_GOOD) {
		fprintf(stderr, PROGRAMME ": sécurité impossible (%s)\n", UA_StatusCode_name(rc));
		return -1;
	}
	config->securityMode = mode;
	UA_String_clear(&config->clientDescription.applicationUri);
	config->clientDescription.applicationUri = UA_STRING_ALLOC(uri ? uri : "urn:sermo:opcua-lire");
	return 0;
}

/* Ce que la lecture réseau a besoin de savoir (voir lire_opcua). */
typedef struct {
	const adresse_t *a;
	UA_MessageSecurityMode mode;
	long delai;
	const UA_NodeId *noeud;
	const char *texte_noeud;
} lecture_opcua_t;

/* Le lecteur de source_lire() (commun.h). Le client est créé ICI, pas avant :
 * en relecture de fond, il vit dans le processus détaché. */
static lecture_t
lire_opcua(void *donnees, FILE *sortie)
{
	lecture_opcua_t *l = donnees;
	UA_ClientConfig config;
	memset(&config, 0, sizeof config);
	config.logging = &journaliseur;
	if (securiser(&config, l->a, l->mode) != 0) {
		UA_ClientConfig_clear(&config);
		return LECTURE_REFUS;
	}
	config.timeout = (UA_UInt32)l->delai;
	UA_Client *client = UA_Client_newWithConfig(&config);
	if (!client) {
		fputs(PROGRAMME ": mémoire épuisée\n", stderr);
		return LECTURE_REFUS;
	}
	lecture_t r = LECTURE_REFUS;
	UA_StatusCode rc = UA_Client_connect(client, l->a->base);
	if (rc != UA_STATUSCODE_GOOD) {
		fprintf(stderr, PROGRAMME ": connexion à %s impossible (%s)\n", l->a->base, UA_StatusCode_name(rc));
		/* Certificat refusé ou politique refusée : pas une panne de réseau,
		 * mais réessayer tout de suite n'y changera rien non plus. */
		r = LECTURE_PANNE;
	} else {
		UA_Variant valeur;
		UA_Variant_init(&valeur);
		rc = UA_Client_readValueAttribute(client, *l->noeud, &valeur);
		if (rc == UA_STATUSCODE_GOOD) {
			ecrire_valeur(sortie, &valeur);
			fputc('\n', sortie);
			r = LECTURE_OK;
		} else {
			fprintf(stderr, PROGRAMME ": %s : %s\n", l->texte_noeud, UA_StatusCode_name(rc));
		}
		UA_Variant_clear(&valeur);
		UA_Client_disconnect(client);
	}
	UA_Client_delete(client);
	return r;
}

int
main(int argc, char **argv)
{
	static const char *const connues[] = { "noeud", "delai", "cache", "perime", "mode", "certificat", "cle", "confiance", "uri", NULL };
	adresse_t a;

	if (argc != 2) {
		fputs("usage : " PROGRAMME " ADRESSE   (appelé par <input opcua>)\n", stderr);
		return 1;
	}
	if (adresse_lire(PROGRAMME, argv[1], &a) != 0)
		return 1;
	int code = 1;
	long delai, cache, perime;
	const char *texte_noeud = adresse_valeur(&a, "noeud");
	const char *texte_mode = adresse_valeur(&a, "mode");
	UA_MessageSecurityMode mode = UA_MESSAGESECURITYMODE_NONE;
	UA_NodeId noeud = UA_NODEID_NULL;

	if (adresse_cles_connues(PROGRAMME, &a, connues) != 0
	    || adresse_entier(PROGRAMME, &a, "delai", 10, 60000, 1500, &delai) != 0
	    || adresse_entier(PROGRAMME, &a, "cache", 0, 60000, CACHE_DEFAUT_MS, &cache) != 0
	    || adresse_entier(PROGRAMME, &a, "perime", 0, 600000, PERIME_DEFAUT_MS, &perime) != 0)
		goto fin;
	if (strncmp(a.base, "opc.tcp://", 10) != 0 || a.base[10] == '\0') {
		fputs(PROGRAMME ": l'adresse doit commencer par opc.tcp://hôte\n", stderr);
		goto fin;
	}
	if (!texte_noeud || UA_NodeId_parse(&noeud, UA_STRING((char *)texte_noeud)) != UA_STATUSCODE_GOOD) {
		fputs(PROGRAMME ": noeud manquant ou mal écrit (exemple : noeud=ns=1;s=temperature)\n", stderr);
		goto fin;
	}
	if (texte_mode && strcmp(texte_mode, "sign") == 0)
		mode = UA_MESSAGESECURITYMODE_SIGN;
	else if (texte_mode && strcmp(texte_mode, "chiffre") == 0)
		mode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
	else if (texte_mode && strcmp(texte_mode, "none") != 0) {
		fputs(PROGRAMME ": mode vaut none, sign ou chiffre\n", stderr);
		goto fin;
	}
	lecture_opcua_t lecture = { &a, mode, delai, &noeud, texte_noeud };
	code = source_lire(PROGRAMME, "opcua", argv[1], a.base, cache, perime, lire_opcua, &lecture);
fin:
	UA_NodeId_clear(&noeud);
	adresse_liberer(&a);
	return code;
}
