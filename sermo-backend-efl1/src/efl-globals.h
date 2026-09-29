/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* efl-globals.h — Variables globales EFL partagées entre widgets
 *
 * haplo-dialog / efl1dialog 1.0.0 — GPL-2.0-or-later
 */
#ifndef EFL_GLOBALS_H
#define EFL_GLOBALS_H

#include <Elementary.h>

/* Fenêtre principale. En EFL un widget appartient au canvas (la fenêtre)
 * où il naît : la machine à pile créant les enfants AVANT le <window>,
 * la fenêtre doit exister d'abord. efl_main_win_get() la crée au premier
 * besoin ; widget_window_create() la CONFIGURE (titre, taille) au lieu
 * d'en créer une seconde. */
extern Evas_Object *g_efl_main_win;
Evas_Object *efl_main_win_get(void);

/* Boîte racine (vbox principale de la fenêtre) */
extern Evas_Object *g_efl_root_box;

#endif /* EFL_GLOBALS_H */
