/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * commun.h — ce que partagent sermo-source-modbus et sermo-source-opcua :
 * lecture de l'adresse tout-en-un, et la pause après un échec.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * ADRESSE. Le cœur (libsermocore/include/sermo_source.h) passe l'adresse en
 * un argument encodé « pour cent ». adresse_lire() la décode, puis la coupe :
 *   base?clé=valeur&clé=valeur…
 * Chaque valeur est décodée une seconde fois : un « & » ou un « % » voulu
 * dans une valeur s'écrit %26 ou %25 dans le XML.
 *
 * PAUSE. sermo attend la fin d'un <input> : un équipement injoignable
 * figerait la fenêtre à chaque rafraîchissement, pour chaque widget. Après un
 * échec, les lectures vers la même base répondent « pas de réponse » sans
 * réseau pendant PAUSE_SECONDES (fichier témoin dans $XDG_RUNTIME_DIR).
 */
#ifndef SERMO_SOURCE_COMMUN_H
#define SERMO_SOURCE_COMMUN_H

#include <stddef.h>
#include <stdio.h>

#define PARAMETRES_MAX 32
#define PAUSE_SECONDES 5

typedef struct {
	char *base;                       /* avant le « ? », décodée */
	size_t n;
	char *cle[PARAMETRES_MAX];
	char *valeur[PARAMETRES_MAX];
	char *tampon;                     /* propriétaire des chaînes ci-dessus */
} adresse_t;

/* Décode et coupe @brut. Rend 0, ou -1 avec un message sur la sortie
 * d'erreur (adresse mal encodée, trop de paramètres, paramètre sans « = »). */
int adresse_lire(const char *programme, const char *brut, adresse_t *a);
void adresse_liberer(adresse_t *a);

/* La valeur de @cle, ou NULL. Une clé donnée deux fois : la dernière. */
const char *adresse_valeur(const adresse_t *a, const char *cle);

/* Refuse toute clé hors de @connues (tableau terminé par NULL) : une faute
 * de frappe (« regsitre ») ne doit pas passer pour la valeur par défaut. */
int adresse_cles_connues(const char *programme, const adresse_t *a, const char *const *connues);

/* Entier décimal dans [min, max] ; @defaut si absent. -1 et message si faux. */
int adresse_entier(const char *programme, const adresse_t *a, const char *cle,
                   long min, long max, long defaut, long *sortie);

/* Pause après échec, par base. pause_active() vrai : ne pas tenter. */
int pause_active(const char *protocole, const char *base);
void pause_poser(const char *protocole, const char *base);
void pause_lever(const char *protocole, const char *base);

/* CACHE COMMUN. Plusieurs widgets lisent souvent la même adresse (une jauge et
 * son texte) : sans cache, chacun ouvrirait sa connexion à chaque tour. Une
 * réponse réussie est gardée @fraicheur_ms (clé cache=, défaut CACHE_DEFAUT_MS ;
 * 0 = pas de cache) sous $XDG_RUNTIME_DIR/sermo-source/, par adresse COMPLÈTE.
 * Les lectures vers un même équipement passent une à une (verrou par base) :
 * le second widget trouve la réponse du premier au lieu de se connecter.
 * Un échec n'est jamais gardé (c'est la pause qui s'en charge). */
#define CACHE_DEFAUT_MS 900

/* Écrit sur la sortie standard une réponse encore fraîche ; rend 1 si servie. */
int cache_servir(const char *protocole, const char *adresse, long fraicheur_ms);
/* Garde @n octets de @donnees comme réponse de @adresse (écriture atomique). */
void cache_garder(const char *protocole, const char *adresse, const char *donnees, size_t n);
/* Verrou exclusif par base, pris en attente ; rend un descripteur, ou -1 si
 * impossible (on continue alors sans verrou). */
int verrou_prendre(const char *protocole, const char *base);
void verrou_rendre(int fd);

/* JAMAIS BLOQUANT. sermo attend la fin d'un <input> : une lecture
 * lente fige la fenêtre. source_lire() décide :
 *  - réponse gardée et fraîche (< cache_ms) : rendue, fini ;
 *  - réponse gardée mais périmée (< perime_ms) : rendue TOUT DE SUITE, et une
 *    relecture part en fond (processus détaché, sortie fermée : sermo ne
 *    l'attend pas) ;
 *  - aucune réponse utilisable, mais une lecture a déjà été tentée pour cette
 *    adresse : rien n'est rendu (code 1, « pas de valeur récente »), une
 *    relecture part en fond ; le tour suivant aura la valeur ;
 *  - toute première lecture de cette adresse : lecture directe (bloquante,
 *    bornée par le délai) — un dialogue sans minuteur affiche quand même sa
 *    valeur ;
 *  - cache_ms = 0 : lecture directe, rien gardé (ancienne conduite).
 * Au-delà de perime_ms, une valeur n'est plus rendue : une valeur figée qui
 * se ferait passer pour l'état présent serait pire qu'une case vide. */
#define PERIME_DEFAUT_MS 10000

typedef enum { LECTURE_OK = 0, LECTURE_REFUS = 1, LECTURE_PANNE = 2 } lecture_t;
/* Lit l'équipement et écrit la réponse dans @sortie. LECTURE_PANNE (équipement
 * injoignable, délai dépassé) pose la pause ; LECTURE_REFUS (registre absent,
 * paramètre faux) ne la pose pas. Le message va sur la sortie d'erreur. */
typedef lecture_t (*lecteur_t)(void *donnees, FILE *sortie);

/* Rend le code de sortie du programme (0 : une valeur a été écrite). */
int source_lire(const char *programme, const char *protocole, const char *adresse,
                const char *base, long cache_ms, long perime_ms, lecteur_t lire, void *donnees);

#endif
