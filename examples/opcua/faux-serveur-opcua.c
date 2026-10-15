/* SPDX-License-Identifier: CC0-1.0
 *
 * faux-serveur-opcua — un serveur OPC UA simulé, pour essayer opcua-lecture.sh
 * sans vrai équipement. Quatre variables, dans l'espace de noms 1, bougent
 * chaque seconde :
 *   ns=1;s=temperature (Double, °C)   ns=1;s=pression (Int32, mbar)
 *   ns=1;s=vitesse     (UInt16, tr/min) ns=1;s=pieces  (UInt32)
 * Toutes en lecture seule pour les clients.
 *
 * Usage : faux-serveur-opcua [port]   (défaut 4841)
 *         faux-serveur-opcua port --securise certificat clé [confiance…]
 * Écoute sur 127.0.0.1 seulement. Sans --securise : sans chiffrement.
 * Avec --securise : « None » ne sert qu'à découvrir les points d'accès, toute
 * lecture exige une politique sûre (comme un vrai serveur d'atelier) ; le
 * certificat porte l'URI urn:sermo:faux-serveur-opcua ; les certificats de
 * client « confiance » sont les seuls admis (aucun : tout client refusé).
 * Ctrl-C l'arrête.
 *
 * Construction : cc -O2 -Wall -o faux-serveur-opcua faux-serveur-opcua.c $(pkg-config --cflags --libs open62541) -lm
 */
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile sig_atomic_t continuer = 1;
static unsigned long secondes;

static void
arreter(int signal)
{
	(void)signal;
	continuer = 0;
}

static void
ajouter(UA_Server *serveur, const char *nom, const UA_DataType *type, const void *valeur)
{
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("fr-FR", (char *)nom);
	attr.accessLevel = UA_ACCESSLEVELMASK_READ;
	attr.dataType = type->typeId;
	UA_Variant_setScalarCopy(&attr.value, valeur, type);
	UA_Server_addVariableNode(serveur, UA_NODEID_STRING(1, (char *)nom),
	                          UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
	                          UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
	                          UA_QUALIFIEDNAME(1, (char *)nom),
	                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
	                          attr, NULL, NULL);
	/* Seule la valeur a été copiée ; nom et texte pointent sur des chaînes fixes. */
	UA_Variant_clear(&attr.value);
}

static void
ecrire(UA_Server *serveur, const char *nom, const UA_DataType *type, void *valeur)
{
	UA_Variant v;
	UA_Variant_setScalar(&v, valeur, type);
	UA_Server_writeValue(serveur, UA_NODEID_STRING(1, (char *)nom), v);
}

/* Appelée chaque seconde par la boucle du serveur. */
static void
faire_bouger(UA_Server *serveur, void *donnees)
{
	(void)donnees;
	unsigned long t = secondes++;
	UA_Double temperature = round(10 * (55 + 15 * sin(t / 10.0))) / 10;
	UA_Int32 pression = (UA_Int32)lround(1013 + 40 * cos(t / 7.0));
	UA_UInt16 vitesse = (UA_UInt16)(1400 + (t * 37) % 200);
	UA_UInt32 pieces = (UA_UInt32)(t / 3);
	ecrire(serveur, "temperature", &UA_TYPES[UA_TYPES_DOUBLE], &temperature);
	ecrire(serveur, "pression", &UA_TYPES[UA_TYPES_INT32], &pression);
	ecrire(serveur, "vitesse", &UA_TYPES[UA_TYPES_UINT16], &vitesse);
	ecrire(serveur, "pieces", &UA_TYPES[UA_TYPES_UINT32], &pieces);
}

/* Lit un fichier entier ; 1 Mio au plus. */
static int
charger(const char *chemin, UA_ByteString *sortie)
{
	FILE *f = fopen(chemin, "rb");
	if (!f) {
		perror(chemin);
		return -1;
	}
	UA_ByteString_init(sortie);
	if (UA_ByteString_allocBuffer(sortie, 1 << 20) != UA_STATUSCODE_GOOD) {
		fclose(f);
		return -1;
	}
	size_t n = fread(sortie->data, 1, 1 << 20, f);
	fclose(f);
	if (n == 0 || n == (1 << 20)) {
		fprintf(stderr, "%s : vide ou trop grand\n", chemin);
		UA_ByteString_clear(sortie);
		return -1;
	}
	sortie->length = n;
	return 0;
}

#define CONFIANCE_MAX 16

int
main(int argc, char **argv)
{
	long port = argc > 1 ? strtol(argv[1], NULL, 10) : 4841;
	if (port < 1024 || port > 65535) {
		fputs("faux-serveur-opcua: port de 1024 à 65535\n", stderr);
		return 1;
	}
	signal(SIGINT, arreter);
	signal(SIGTERM, arreter);

	/* La configuration est faite AVANT le serveur : reconfigurer celle d'un
	 * serveur déjà créé y laisse la politique None et ses jetons anonymes. */
	UA_ServerConfig conf;
	memset(&conf, 0, sizeof conf);
	UA_ServerConfig *config = &conf;
	if (argc > 2 && strcmp(argv[2], "--securise") == 0) {
		size_t nconfiance = argc > 5 ? (size_t)(argc - 5) : 0;
		if (argc < 5 || nconfiance > CONFIANCE_MAX) {
			fputs("faux-serveur-opcua: --securise certificat clé [confiance… (16 au plus)]\n", stderr);
			return 1;
		}
		UA_ByteString cert, cle, liste[CONFIANCE_MAX];
		if (charger(argv[3], &cert) || charger(argv[4], &cle))
			return 1;
		for (size_t j = 0; j < nconfiance; j++)
			if (charger(argv[5 + j], &liste[j]))
				return 1;
		UA_StatusCode rc = UA_ServerConfig_setDefaultWithSecurityPolicies(
			config, (UA_UInt16)port, &cert, &cle, liste, nconfiance, NULL, 0, NULL, 0);
		UA_ByteString_clear(&cert);
		UA_ByteString_clear(&cle);
		for (size_t j = 0; j < nconfiance; j++)
			UA_ByteString_clear(&liste[j]);
		if (rc != UA_STATUSCODE_GOOD) {
			fprintf(stderr, "faux-serveur-opcua: sécurité impossible (%s)\n", UA_StatusCode_name(rc));
			return 1;
		}
		/* Comme un vrai serveur d'atelier : None ne sert qu'à découvrir les
		 * points d'accès ; lire une variable exige une politique sûre. */
		config->securityPolicyNoneDiscoveryOnly = true;
		UA_String_clear(&config->applicationDescription.applicationUri);
		config->applicationDescription.applicationUri = UA_STRING_ALLOC("urn:sermo:faux-serveur-opcua");
	} else {
		UA_ServerConfig_setMinimal(config, (UA_UInt16)port, NULL);
	}
	fprintf(stderr, "politiques :");
	for (size_t j = 0; j < config->securityPoliciesSize; j++)
		fprintf(stderr, " %.*s", (int)config->securityPolicies[j].policyUri.length,
		        (const char *)config->securityPolicies[j].policyUri.data);
	fputc('\n', stderr);
	/* Écoute sur la machine seule. */
	char url[64];
	snprintf(url, sizeof url, "opc.tcp://127.0.0.1:%ld", port);
	UA_Array_delete(config->serverUrls, config->serverUrlsSize, &UA_TYPES[UA_TYPES_STRING]);
	config->serverUrls = UA_String_new();
	*config->serverUrls = UA_STRING_ALLOC(url);
	config->serverUrlsSize = 1;
	UA_Server *serveur = UA_Server_newWithConfig(config);
	if (!serveur)
		return 1;

	UA_Double d = 0; UA_Int32 i32 = 0; UA_UInt16 u16 = 0; UA_UInt32 u32 = 0;
	ajouter(serveur, "temperature", &UA_TYPES[UA_TYPES_DOUBLE], &d);
	ajouter(serveur, "pression", &UA_TYPES[UA_TYPES_INT32], &i32);
	ajouter(serveur, "vitesse", &UA_TYPES[UA_TYPES_UINT16], &u16);
	ajouter(serveur, "pieces", &UA_TYPES[UA_TYPES_UINT32], &u32);
	faire_bouger(serveur, NULL);
	UA_Server_addRepeatedCallback(serveur, faire_bouger, NULL, 1000, NULL);

	printf("faux serveur OPC UA sur %s\n", url);
	fflush(stdout);
	UA_StatusCode rc = UA_Server_run_startup(serveur);
	while (rc == UA_STATUSCODE_GOOD && continuer)
		UA_Server_run_iterate(serveur, true);
	UA_Server_run_shutdown(serveur);
	UA_Server_delete(serveur);
	return rc == UA_STATUSCODE_GOOD ? 0 : 1;
}
