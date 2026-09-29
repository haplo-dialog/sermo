/* render_ncurses.h — point d'entrée de la boucle de rendu ncurses.
 * SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef RENDER_NCURSES_H
#define RENDER_NCURSES_H
#include "dialog_state.h"   /* WidgetNode */
#ifdef __cplusplus
extern "C" {
#endif
/* Rend l'arbre `root` et tient la boucle d'évènements. En l'absence de terminal
 * interactif (pipe, banc), bascule en mode headless : exécute la logique du
 * programme (timers → actions du cœur) sans dessiner. Renvoie le code de sortie
 * (le cœur peut aussi sortir directement via action_exitprogram). */
int render_loop(WidgetNode *root, const char *title, int win_w, int win_h);
#ifdef __cplusplus
}
#endif
#endif /* RENDER_NCURSES_H */
