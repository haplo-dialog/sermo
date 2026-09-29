/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* image_load.h — decodage d'icone en RGBA (gdk-pixbuf), sans GLib visible */
#ifndef SERMO_IMAGE_LOAD_H
#define SERMO_IMAGE_LOAD_H
#ifdef __cplusplus
extern "C" {
#endif
/* RGBA 8 bits, `size` px de cote (0 = taille native) ; free() par l'appelant. */
unsigned char *sermo_image_load_rgba(const char *path, int size, int *w, int *h);
#ifdef __cplusplus
}
#endif
#endif
