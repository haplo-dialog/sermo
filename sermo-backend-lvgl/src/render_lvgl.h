/* render_lvgl.h — point d'entrée de la boucle de rendu LVGL.
 * SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef RENDER_LVGL_H
#define RENDER_LVGL_H
#include "dialog_state.h"   /* WidgetNode */
#ifdef __cplusplus
extern "C" {
#endif
/* Rend l'arbre `root` dans une fenêtre LVGL et tient la boucle d'évènements.
 * Sans affichage (pas de DISPLAY, ou SERMO_LVGL_BATCH=1) : exécute la logique
 * du programme (minuteries → actions du cœur) sans dessiner. Renvoie le code de
 * sortie (le cœur peut aussi sortir directement via action_exitprogram). */
int render_loop(WidgetNode *root, const char *title, int win_w, int win_h);
#ifdef __cplusplus
}
#endif
#endif /* RENDER_LVGL_H */
