/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_images.h — le fichier d'images d'un lecteur <action media> rendu
 * « dans la zone » (dans=zone).
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Le décodeur (sermo-media-images, libmpv, dans la cage bwrap, SANS accès à
 * l'écran) dessine chaque image en BGRX 8 bits (« bgr0 » de mpv, CAIRO_FORMAT_RGB24
 * sur une machine petit-boutiste) dans l'un des deux tampons du fichier
 * $XDG_RUNTIME_DIR/sermo-media/NOM/images, puis publie son numéro dans
 * l'en-tête. Le port (un <drawingarea media="NOM">) lit l'en-tête, puis le
 * tampon « courant ».
 *
 * Le port LIT des données venues d'un processus non fiable : chaque champ est
 * borné avant usage, et la lecture se fait par pread (jamais mmap : un fichier
 * tronqué par le décodeur ferait tomber le port par SIGBUS).
 */
#ifndef SERMO_IMAGES_H
#define SERMO_IMAGES_H

#include <stdint.h>

#define SERMO_IMAGES_MAGIE      "SERMOIMG"      /* 8 octets, sans NUL */
#define SERMO_IMAGES_VERSION    1u
#define SERMO_IMAGES_LARGEUR_MAX 1280u
#define SERMO_IMAGES_HAUTEUR_MAX 720u
#define SERMO_IMAGES_DONNEES    64u             /* début du tampon 0 */

struct sermo_images_entete {
	char     magie[8];          /* SERMO_IMAGES_MAGIE */
	uint32_t version;           /* SERMO_IMAGES_VERSION */
	uint32_t largeur;           /* 1 .. SERMO_IMAGES_LARGEUR_MAX */
	uint32_t hauteur;           /* 1 .. SERMO_IMAGES_HAUTEUR_MAX */
	uint32_t pas;               /* octets par ligne : largeur * 4, exactement */
	uint32_t courant;           /* 0 ou 1 : tampon de la dernière image complète */
	uint32_t reserve;
	uint64_t sequence;          /* +1 à chaque image publiée ; 0 = aucune encore */
};

/* Le tampon n (0 ou 1) commence à SERMO_IMAGES_DONNEES + n * hauteur * pas ;
 * le fichier fait SERMO_IMAGES_DONNEES + 2 * hauteur * pas octets. */

#endif
