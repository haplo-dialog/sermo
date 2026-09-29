/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* efl-globals.c — Définitions des variables globales EFL
 *
 * haplo-dialog / efl1dialog 1.0.0 — GPL-2.0-or-later
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include "efl-globals.h"

Evas_Object *g_efl_main_win  = NULL;
Evas_Object *g_efl_root_box  = NULL;

/* Parent de construction = LA fenêtre, créée au premier besoin.
 * elm_init() est déjà passé (main l'appelle avant l'automate). */
Evas_Object *efl_main_win_get(void)
{
    if (!g_efl_main_win) {
        /* Echelle : le texte elm par defaut est minuscule a 96 dpi ;
         * ELM_SCALE (lu par elm lui-meme) garde la main si pose. */
        if (!getenv("ELM_SCALE")) elm_config_scale_set(1.2);
        g_efl_main_win = elm_win_add(NULL, "efl1sermo", ELM_WIN_BASIC);
        elm_win_autodel_set(g_efl_main_win, EINA_TRUE);
        /* fermer la derniere fenetre termine la boucle (comme GTK) */
        elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
    }
    return g_efl_main_win;
}
