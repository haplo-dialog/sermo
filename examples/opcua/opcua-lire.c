/* SPDX-License-Identifier: CC0-1.0
 *
 * opcua-lire — lit la valeur d'une ou plusieurs variables d'un serveur OPC UA
 * et l'écrit, une ligne par nœud : « nœud valeur ». Lecture SEULE : ce
 * programme n'appelle aucun service d'écriture ni de méthode.
 *
 * Usage : opcua-lire [options] opc.tcp://hôte:port nœud [nœud…]
 *   nœud : notation OPC UA, par exemple « ns=1;s=temperature » ou « ns=0;i=2258 »
 *   -t ms       délai de réponse en millisecondes (défaut 2000)
 *   -m mode     none (défaut), sign (signé) ou chiffre (signé et chiffré)
 *   -c fichier  certificat de ce client (DER ou PEM)        } obligatoires
 *   -k fichier  clé privée de ce client (DER ou PEM)        } avec sign
 *   -T fichier  certificat de serveur (ou d'autorité) de    } et chiffre
 *               confiance ; répétable, jusqu'à 16
 *   -u uri      ApplicationUri du client ; doit être celle du certificat
 *               (défaut urn:sermo:opcua-lire)
 *
 * Sortie : un nœud lu donne « nœud valeur » ; un nœud illisible donne
 * « nœud ERREUR <code OPC UA> » (code de sortie 2). Sans connexion : un
 * message sur la sortie d'erreur et le code 1. Les journaux de la
 * bibliothèque vont sur la sortie d'erreur, jamais sur la sortie standard.
 *
 * Sans -m, connexion sans chiffrement (politique « None ») : c'est ce que la
 * plupart des serveurs d'atelier acceptent en lecture. Avec -m sign ou
 * -m chiffre, le client présente son certificat et ne fait confiance QU'AUX
 * certificats donnés par -T : il n'existe pas de mode « tout accepter », qui
 * laisserait n'importe quel intermédiaire se faire passer pour le serveur.
 * La politique de sécurité (Basic256Sha256, Aes128…) est la meilleure que le
 * serveur propose pour ce mode.
 *
 * Construction : cc -O2 -Wall -o opcua-lire opcua-lire.c $(pkg-config --cflags --libs open62541)
 * Dépend de libopen62541-1.4-dev (Debian).
 */
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Journal de la bibliothèque : erreurs seulement, sur la sortie d'erreur. */
static void
journal(void *contexte, UA_LogLevel niveau, UA_LogCategory categorie,
        const char *message, va_list arguments)
{
	(void)contexte;
	(void)categorie;
	if (niveau < UA_LOGLEVEL_ERROR)
		return;
	fputs("opcua-lire: ", stderr);
	vfprintf(stderr, message, arguments);
	fputc('\n', stderr);
}

static UA_Logger journaliseur = { journal, NULL, NULL };

/* Écrit une valeur scalaire lisible ; un tableau ou un type composé passe par
 * UA_print (forme textuelle de la bibliothèque). Une chaîne garde ses octets,
 * sauf les fins de ligne, remplacées par une espace (une valeur = une ligne). */
static void
ecrire_valeur(const UA_Variant *v)
{
	if (UA_Variant_isEmpty(v)) {
		fputs("(vide)", stdout);
		return;
	}
	if (UA_Variant_isScalar(v)) {
		const void *d = v->data;
		const UA_DataType *t = v->type;
		if (t == &UA_TYPES[UA_TYPES_BOOLEAN])  { fputs(*(const UA_Boolean *)d ? "true" : "false", stdout); return; }
		if (t == &UA_TYPES[UA_TYPES_SBYTE])    { printf("%d", *(const UA_SByte *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_BYTE])     { printf("%u", *(const UA_Byte *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_INT16])    { printf("%d", *(const UA_Int16 *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_UINT16])   { printf("%u", *(const UA_UInt16 *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_INT32])    { printf("%" PRId32, *(const UA_Int32 *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_UINT32])   { printf("%" PRIu32, *(const UA_UInt32 *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_INT64])    { printf("%" PRId64, *(const UA_Int64 *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_UINT64])   { printf("%" PRIu64, *(const UA_UInt64 *)d); return; }
		/* %.7g / %.15g : les chiffres que le type garde vraiment (60.8, pas
		 * 60.799999999999997) ; toujours un point, le programme reste en locale C. */
		if (t == &UA_TYPES[UA_TYPES_FLOAT])    { printf("%.7g", (double)*(const UA_Float *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_DOUBLE])   { printf("%.15g", *(const UA_Double *)d); return; }
		if (t == &UA_TYPES[UA_TYPES_DATETIME]) {
			/* Heure UTC, forme ISO 8601. */
			UA_DateTimeStruct h = UA_DateTime_toStruct(*(const UA_DateTime *)d);
			printf("%04d-%02u-%02uT%02u:%02u:%02u.%03uZ", h.year, h.month, h.day,
			       h.hour, h.min, h.sec, h.milliSec);
			return;
		}
		if (t == &UA_TYPES[UA_TYPES_STRING]) {
			const UA_String *s = d;
			for (size_t i = 0; i < s->length; i++)
				putchar(s->data[i] == '\n' || s->data[i] == '\r' ? ' ' : s->data[i]);
			return;
		}
	}
	UA_String texte = UA_STRING_NULL;
	if (UA_print(v, &UA_TYPES[UA_TYPES_VARIANT], &texte) == UA_STATUSCODE_GOOD) {
		for (size_t i = 0; i < texte.length; i++)
			putchar(texte.data[i] == '\n' || texte.data[i] == '\r' ? ' ' : texte.data[i]);
	} else {
		fputs("(illisible)", stdout);
	}
	UA_String_clear(&texte);
}

static void
usage(void)
{
	fputs("usage : opcua-lire [-t délai_ms] [-m none|sign|chiffre -c certificat -k clé -T confiance…]\n"
	      "                   [-u uri] opc.tcp://hôte:port nœud [nœud…]\n", stderr);
	exit(1);
}

/* Lit un fichier entier (certificat, clé) ; 1 Mio au plus. */
static int
charger(const char *chemin, UA_ByteString *sortie)
{
	FILE *f = fopen(chemin, "rb");
	if (!f) {
		fprintf(stderr, "opcua-lire: %s : %s\n", chemin, strerror(errno));
		return -1;
	}
	unsigned char *tampon = malloc(1 << 20);
	size_t n = tampon ? fread(tampon, 1, 1 << 20, f) : 0;
	int trop = tampon && n == (1 << 20) && fgetc(f) != EOF;
	int erreur = ferror(f);
	fclose(f);
	if (!tampon || erreur || trop || n == 0) {
		fprintf(stderr, "opcua-lire: %s : fichier illisible, vide ou trop grand\n", chemin);
		free(tampon);
		return -1;
	}
	UA_ByteString_init(sortie);
	if (UA_ByteString_allocBuffer(sortie, n) != UA_STATUSCODE_GOOD) {
		free(tampon);
		return -1;
	}
	memcpy(sortie->data, tampon, n);
	/* La clé privée ne traîne pas dans un tampon libéré. */
	memset(tampon, 0, n);
	free(tampon);
	return 0;
}

#define CONFIANCE_MAX 16

int
main(int argc, char **argv)
{
	UA_UInt32 delai = 2000;
	UA_MessageSecurityMode mode = UA_MESSAGESECURITYMODE_NONE;
	const char *certificat = NULL, *cle = NULL, *uri = "urn:sermo:opcua-lire";
	const char *confiance[CONFIANCE_MAX];
	size_t nconfiance = 0;
	int opt;

	while ((opt = getopt(argc, argv, "+t:m:c:k:T:u:")) != -1) {
		switch (opt) {
		case 't': {
			char *fin;
			errno = 0;
			unsigned long n = strtoul(optarg, &fin, 10);
			if (errno || *fin || n < 10 || n > 60000) {
				fputs("opcua-lire: -t attend un délai de 10 à 60000 ms\n", stderr);
				return 1;
			}
			delai = (UA_UInt32)n;
			break;
		}
		case 'm':
			if (strcmp(optarg, "none") == 0)
				mode = UA_MESSAGESECURITYMODE_NONE;
			else if (strcmp(optarg, "sign") == 0)
				mode = UA_MESSAGESECURITYMODE_SIGN;
			else if (strcmp(optarg, "chiffre") == 0)
				mode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
			else {
				fputs("opcua-lire: -m attend none, sign ou chiffre\n", stderr);
				return 1;
			}
			break;
		case 'c': certificat = optarg; break;
		case 'k': cle = optarg; break;
		case 'u': uri = optarg; break;
		case 'T':
			if (nconfiance == CONFIANCE_MAX) {
				fputs("opcua-lire: 16 certificats de confiance au plus\n", stderr);
				return 1;
			}
			confiance[nconfiance++] = optarg;
			break;
		default:
			usage();
		}
	}
	if (argc - optind < 2)
		usage();
	if (mode == UA_MESSAGESECURITYMODE_NONE && (certificat || cle || nconfiance)) {
		fputs("opcua-lire: -c, -k et -T demandent -m sign ou -m chiffre\n", stderr);
		return 1;
	}
	if (mode != UA_MESSAGESECURITYMODE_NONE && (!certificat || !cle || nconfiance == 0)) {
		fputs("opcua-lire: -m sign et -m chiffre demandent -c, -k et au moins un -T\n", stderr);
		return 1;
	}
	int i = optind;
	const char *adresse = argv[i++];
	if (strncmp(adresse, "opc.tcp://", 10) != 0) {
		fputs("opcua-lire: l'adresse doit commencer par opc.tcp://\n", stderr);
		return 1;
	}

	UA_ClientConfig config;
	memset(&config, 0, sizeof config);
	config.logging = &journaliseur;
	UA_StatusCode rc;
	if (mode == UA_MESSAGESECURITYMODE_NONE) {
		rc = UA_ClientConfig_setDefault(&config);
	} else {
		UA_ByteString cert, clef, liste[CONFIANCE_MAX];
		UA_ByteString_init(&cert);
		UA_ByteString_init(&clef);
		for (size_t j = 0; j < nconfiance; j++)
			UA_ByteString_init(&liste[j]);
		int ok = charger(certificat, &cert) == 0 && charger(cle, &clef) == 0;
		for (size_t j = 0; ok && j < nconfiance; j++)
			ok = charger(confiance[j], &liste[j]) == 0;
		rc = ok ? UA_ClientConfig_setDefaultEncryption(&config, cert, clef, liste, nconfiance, NULL, 0)
		        : UA_STATUSCODE_BADINTERNALERROR;
		/* La bibliothèque a fait ses copies : on efface et on libère les nôtres. */
		UA_ByteString_clear(&cert);
		if (clef.data)
			memset(clef.data, 0, clef.length);
		UA_ByteString_clear(&clef);
		for (size_t j = 0; j < nconfiance; j++)
			UA_ByteString_clear(&liste[j]);
		if (!ok)
			return 1;
		config.securityMode = mode;
		UA_String_clear(&config.clientDescription.applicationUri);
		config.clientDescription.applicationUri = UA_STRING_ALLOC(uri);
	}
	if (rc != UA_STATUSCODE_GOOD) {
		fprintf(stderr, "opcua-lire: configuration impossible (%s)\n", UA_StatusCode_name(rc));
		UA_ClientConfig_clear(&config);
		return 1;
	}
	config.timeout = delai;
	UA_Client *client = UA_Client_newWithConfig(&config);
	if (!client) {
		fputs("opcua-lire: mémoire épuisée\n", stderr);
		return 1;
	}

	rc = UA_Client_connect(client, adresse);
	if (rc != UA_STATUSCODE_GOOD) {
		fprintf(stderr, "opcua-lire: connexion à %s impossible (%s)\n", adresse, UA_StatusCode_name(rc));
		UA_Client_delete(client);
		return 1;
	}
	int code = 0;
	for (; i < argc; i++) {
		UA_NodeId noeud;
		/* Le nœud vient de la ligne de commande : on le recopie tel quel en
		 * sortie, sans fin de ligne possible (argv n'en contient pas d'utile). */
		if (strpbrk(argv[i], "\n\r") || UA_NodeId_parse(&noeud, UA_STRING(argv[i])) != UA_STATUSCODE_GOOD) {
			printf("%s ERREUR nœud-mal-écrit\n", strpbrk(argv[i], "\n\r") ? "?" : argv[i]);
			code = 2;
			continue;
		}
		UA_Variant valeur;
		UA_Variant_init(&valeur);
		rc = UA_Client_readValueAttribute(client, noeud, &valeur);
		if (rc == UA_STATUSCODE_GOOD) {
			printf("%s ", argv[i]);
			ecrire_valeur(&valeur);
			putchar('\n');
		} else {
			printf("%s ERREUR %s\n", argv[i], UA_StatusCode_name(rc));
			code = 2;
		}
		UA_Variant_clear(&valeur);
		UA_NodeId_clear(&noeud);
	}

	UA_Client_disconnect(client);
	UA_Client_delete(client);
	return fflush(stdout) == 0 ? code : 1;
}
