/*
 * gtk4-compat.c — GTK3→GTK4 compatibility backing store
 *
 * Provides the out-of-line definitions required by gtk4-compat.h.
 * Currently only the global main loop pointer; all other shims are
 * static inlines or macros defined entirely in the header.
 *
 * This file must be compiled into the gtk4sermo binary.
 * It is listed in SOURCES in src/Makefile.am.
 *
 * Haplo-Linux <devel@haplo-dialog.fr> — 2026
 * License: GPL-2.0-or-later
 */

#include "gtk4-compat.h"

/* Global GMainLoop shared by all translation units via extern in gtk4-compat.h.
 * gtk_main()      → _compat_gtk_main()      creates and runs this loop.
 * gtk_main_quit() → _compat_gtk_main_quit() quits and NULLs it.          */
GMainLoop *_gtk4sermo_main_loop = NULL;
