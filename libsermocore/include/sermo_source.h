/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_source.h — <input modbus> et <input opcua> : lire un automate, en
 * lecture SEULE, par une adresse tout-en-un.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * POURQUOI. Lire un registre Modbus ou une variable OPC UA demandait un script
 * autour d'un outil externe (examples/modbus, examples/opcua). Ces balises le
 * font directement :
 *
 *   <input modbus>tcp://hôte:502?esclave=1&registre=1&diviseur=10</input>
 *   <input opcua>opc.tcp://hôte:4840?noeud=ns=1;s=temperature</input>
 *
 * COMMENT. L'analyseur traduit la balise en une commande ordinaire qui lance
 * un PROGRAMME D'AIDE (sermo-source-modbus, sermo-source-opcua), installé dans
 * SERMO_SOURCE_DIR (défaut /usr/libexec/sermo). Les neuf ports n'y voient
 * qu'un <input> de plus : même limite de taille, même liste blanche
 * SERMO_ALLOWED_CMDS (qui doit alors nommer le programme d'aide). Aucun port
 * n'est modifié, et le cœur ne dépend ni de libmodbus ni d'open62541 : chaque
 * programme d'aide vit dans son propre paquet.
 *
 * CONTRAT.
 *  - L'adresse est transmise en UN argument, encodée « pour cent » : tout octet
 *    hors de [A-Za-z0-9._:/=,+@-] devient %XX (le « % » lui-même aussi). La
 *    commande n'a donc aucun métacaractère : elle part en argv, jamais par
 *    /bin/sh, même avec SERMO_NO_SHELL_FALLBACK ou SERMO_ALLOWED_CMDS.
 *    Le programme d'aide décode une fois et retrouve l'adresse telle qu'écrite.
 *  - Refusé à l'analyse (erreur et arrêt, comme une balise mal formée) :
 *    protocole inconnu, adresse vide, plus de 4096 octets, caractère de
 *    contrôle, dossier des programmes d'aide relatif ou porteur d'un caractère
 *    hors de l'ensemble sûr.
 *  - Lecture seule : aucun des deux programmes d'aide n'a de code d'écriture.
 */
#ifndef SERMO_SOURCE_H
#define SERMO_SOURCE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dossier des programmes d'aide quand SERMO_SOURCE_DIR n'est pas posée. */
#ifndef SERMO_SOURCE_DIR_DEFAUT
#define SERMO_SOURCE_DIR_DEFAUT "/usr/libexec/sermo"
#endif

/* Longueur maximale d'une adresse, en octets (avant encodage). */
#define SERMO_SOURCE_ADRESSE_MAX 4096

/* Traduit <input @protocole>@adresse</input> en commande. @protocole vaut
 * "modbus" ou "opcua". Rend une chaîne allouée par malloc() (à libérer par
 * free()), ou NULL avec la raison, en français, dans @erreur (@taille octets). */
char *sermo_source_commande(const char *protocole, const char *adresse,
                            char *erreur, size_t taille);

#ifdef __cplusplus
}
#endif

#endif /* SERMO_SOURCE_H */
