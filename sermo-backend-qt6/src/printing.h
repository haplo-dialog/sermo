/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * printing.h — messages de diagnostic (implémentation indépendante)
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */

#ifndef SERMO_QT6_PRINTING_H
#define SERMO_QT6_PRINTING_H

#include "gtk3d.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Messages de diagnostic à l'usage du développeur du backend Qt6 uniquement
 * (jamais appelés en fonctionnement normal ; appel protégé en amont par une
 * macro compilée conditionnellement). Écrivent toujours sur stderr : stdout
 * reste réservé aux lignes NOM="valeur" exportées par ce programme.
 */
void pip_message_print_debug(const gchar *function, const gchar *format, ...)
    __attribute__((format(printf, 2, 3)));

void pip_message_print_warning(const gchar *function, const gchar *format, ...)
    __attribute__((format(printf, 2, 3)));

/*
 * Signale qu'une fonctionnalité demandée par un script XML n'est pas prise
 * en charge par ce backend : affiche une boîte modale d'erreur (bloquante)
 * identifiant la fonction, le fichier et la ligne en cause.
 */
void unimplemented(const char *function_name, const char *filename, gint linenumber);

#ifdef __cplusplus
}
#endif

#endif /* SERMO_QT6_PRINTING_H */
