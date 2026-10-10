/* render_html.h — point d'entrée de l'export HTML statique.
 * SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef RENDER_HTML_H
#define RENDER_HTML_H
#include <stdio.h>
#include "dialog_state.h"   /* WidgetNode */
#ifdef __cplusplus
extern "C" {
#endif
/* Sérialise l'arbre `root` en une page HTML autonome (CSS embarqué, un seul
 * fichier). Écrit dans le fichier désigné par la variable d'environnement
 * SERMO_HTML_OUT ("-" = stdout ; défaut "sermo-output.html"). Ne dessine rien
 * à l'écran, n'exécute aucune action : pur jalon de faisabilité (export
 * statique), pas encore d'interactivité ni de protocole live. */
void render_html_write(FILE *out, WidgetNode *root, const char *title);
int render_html_export(WidgetNode *root, const char *title);
int html_mode(void);   /* 0 export · 1 interactive · 2 serve */
#ifdef __cplusplus
}
#endif
#endif /* RENDER_HTML_H */
