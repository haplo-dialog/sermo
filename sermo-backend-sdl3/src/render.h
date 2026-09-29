/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* render.h — Interface de la boucle de rendu ImGui pour sdl3sermo
 *
 * sermo
 * Licence : GPL-2.0-or-later
 */

#ifndef RENDER_H
#define RENDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dialog_state.h"

/**
 * render_loop() — Initialise SDL3 + OpenGL + ImGui et entre dans la boucle
 *                 principale. Bloque jusqu'à fermeture de la fenêtre.
 *
 * @param root    Racine de l'arbre WidgetNode construit par le parser XML
 * @param title   Titre de la fenêtre SDL3 (NULL → "sdl3sermo")
 * @param win_w   Largeur initiale en pixels (0 → 800)
 * @param win_h   Hauteur initiale en pixels (0 → 600)
 * @return        0 si succès, 1 si erreur SDL3/GL
 */
int render_loop(WidgetNode *root, const char *title, int win_w, int win_h);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_H */
