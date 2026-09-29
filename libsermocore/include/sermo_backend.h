/* SPDX-License-Identifier: GPL-2.0-or-later */
/* sermo_backend.h — contrat cœur→backend.
 * Le cœur (libsermocore) pilote tout SAUF l'init du toolkit : chaque backend
 * fournit cette fonction (gtk_init / QApplication / SDL_Init / …). En mode
 * --print-ir, le backend doit s'initialiser SANS ouvrir d'affichage. */
#ifndef SERMO_BACKEND_H
#define SERMO_BACKEND_H
#ifdef __cplusplus
extern "C" {
#endif
void sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir);
#ifdef __cplusplus
}
#endif
#endif
