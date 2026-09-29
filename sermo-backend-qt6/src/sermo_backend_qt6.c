/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* sermo_backend_qt6.c — hook d'init toolkit du backend qt6.
 * Le cœur (libsermocore) appelle sermo_backend_toolkit_init() à la place de
 * gtk_init. qt6-compat.h (force-inclus) mappe gtk_init → sermo_be_app_init, qui crée
 * la QApplication SAUF en --print-ir. */
#include "sermo-contract.h"   /* contrat cœur <-> backend (MIT) */

void sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)
{
    (void) print_ir;              /* sermo_be_app_init consulte option_print_ir lui-même */
    gtk_init(argc, argv);         /* = sermo_be_app_init(argc, argv) via le shim */
}
