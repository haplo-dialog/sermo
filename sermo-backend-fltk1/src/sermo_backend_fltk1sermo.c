/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* sermo_backend_fltk1sermo.c — hook d'init toolkit du backend fltk1.
 *
 * Le coeur (libsermocore) pilote tout SAUF l'init du toolkit : il appelle
 * sermo_backend_toolkit_init() a la place de gtk_init. Pour la famille FLTK,
 * l'init du visuel est fltk_visual_init() (definie dans fltk-compat.cpp,
 * extern "C") — c'est exactement ce que faisait l'ancien main() du port au
 * label gtkdialog_initialized.
 *
 * En mode --print-ir (print_ir=1) : fltk_visual_init() s'abstient d'ouvrir
 * l'affichage quand DISPLAY est absent (cf. fltk-compat.cpp : il ne fait que
 * Fl::visual + fl_register_images, sans get_system_colors qui, lui, ouvrirait
 * le display et avorterait « Can't open display »). Aucun garde supplementaire
 * n'est donc requis ici.
 *
 * NB (specifique fltk1) : les globales de visibilite widget_show_list /
 * widget_hide_list que le coeur declare extern sont DEJA definies par
 * widgets.cpp de ce port (compile dans ce backend). On ne les redefinit donc
 * PAS ici, contrairement au backend gtk3 dont widgets.c ne les porte pas.
 */
#include "sermo-contract.h"   /* contrat cœur <-> backend (MIT) */

/* Defini dans fltk-compat.cpp (extern "C"). */
void fltk_visual_init(void);

void sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)
{
    (void) argc;
    (void) argv;
    (void) print_ir;   /* fltk_visual_init consulte DISPLAY lui-meme. */
    fltk_visual_init();
}
