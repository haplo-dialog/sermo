/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* efl_menu_model.h — modèle intermédiaire de menu (efl1sermo, C)
 * Porté en evas_object_data_set() par les carriers poussés sur la pile :
 *   <menuitem> → EMenuItem ("emi"), <menu> → EMenu ("emenu").
 * Le <menubar> les consomme pour bâtir le toolbar + elm_menu réels.
 * GPL-2.0-or-later */
#ifndef EFL_MENU_MODEL_H
#define EFL_MENU_MODEL_H

typedef struct {
    char *label;   /* possédé */
    char *icon;    /* nom d'icône de thème (ou NULL) — possédé */
    char *cmd;     /* commande d'action (ou NULL) — possédé */
    int   separator;
    int   has_check;
    int   check_val;
} EMenuItem;

typedef struct {
    char      *label;   /* possédé */
    EMenuItem *items;   /* tableau — possédé */
    int        n;
} EMenu;

#endif
