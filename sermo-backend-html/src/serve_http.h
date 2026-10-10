/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* serve_http.h — micro-serveur local et éphémère du backend HTML.
 *
 * Sert UNE page, attend UN envoi de formulaire, écrit les valeurs sur la
 * sortie standard, et sort. C'est le cycle de vie d'une fenêtre de dialogue,
 * transposé au navigateur : le script appelant récupère ses valeurs comme avec
 * n'importe quel autre backend.
 */
#ifndef SERMO_SERVE_HTTP_H
#define SERMO_SERVE_HTTP_H

/* Sert `page` (taille `len`) et bloque jusqu'à l'envoi du formulaire, la
 * fermeture du navigateur, ou l'expiration du délai.
 * Renvoie 0 si un formulaire a été reçu et imprimé, 1 sinon. */
int html_serve_once(const char *page, unsigned long len);

#endif
