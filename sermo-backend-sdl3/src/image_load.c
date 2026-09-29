/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * image_load.c — decodage d'une icone (svg/png/xpm) en RGBA via gdk-pixbuf
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Fichier ISOLE : il inclut la vraie GLib (via gdk-pixbuf) et ne doit
 * jamais voir sdl3-compat.h, qui redefinit les types GLib.
 */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <string.h>
#include "image_load.h"

unsigned char *sermo_image_load_rgba(const char *path, int size, int *w, int *h)
{
    GError *err = NULL;
    GdkPixbuf *pb = size > 0
        ? gdk_pixbuf_new_from_file_at_scale(path, size, size, TRUE, &err)
        : gdk_pixbuf_new_from_file(path, &err);
    if (!pb) { if (err) g_error_free(err); return NULL; }
    if (!gdk_pixbuf_get_has_alpha(pb)) {
        GdkPixbuf *a = gdk_pixbuf_add_alpha(pb, FALSE, 0, 0, 0);
        g_object_unref(pb);
        pb = a;
    }
    int pw = gdk_pixbuf_get_width(pb), ph = gdk_pixbuf_get_height(pb);
    int stride = gdk_pixbuf_get_rowstride(pb);
    const guchar *src = gdk_pixbuf_read_pixels(pb);
    unsigned char *out = (unsigned char *)malloc((size_t)pw * ph * 4);
    if (out)
        for (int y = 0; y < ph; y++)
            memcpy(out + (size_t)y * pw * 4, src + (size_t)y * stride, (size_t)pw * 4);
    g_object_unref(pb);
    *w = pw; *h = ph;
    return out;
}
