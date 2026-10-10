/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * sermo_open_uri.h — ouverture d'une URI dans l'application par défaut.
 *
 * SANS SHELL. L'URI vient du script XML : la passer à system() ou à
 * safe_system() la ferait transiter par /bin/sh dès qu'elle contient un
 * métacaractère (« & » est banal dans une URL de requête). On lance donc
 * xdg-open par fork + execlp, l'URI en argv[1] : aucune interprétation.
 * Une URI commençant par « - » est refusée (elle serait lue comme une option).
 */
#ifndef SERMO_OPEN_URI_H
#define SERMO_OPEN_URI_H
void sermo_open_uri(const char *uri);
#endif /* SERMO_OPEN_URI_H */
