/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* sermo_menu_model.h — modèle intermédiaire de menu (fltk1sermo)
 * Porté en user_data() par les carriers poussés sur la pile :
 *   <menuitem> → SermoMenuCarrier{kind=0}, <menu> → SermoMenuCarrier{kind=1}.
 * Le <menubar> les consomme pour bâtir le Fl_Menu_Bar réel.
 * C++ uniquement. GPL-2.0-or-later. */
#ifndef SERMO_MENU_MODEL_H
#define SERMO_MENU_MODEL_H
#ifdef __cplusplus
#include <string>
#include <vector>

struct SermoMenuItem {
    std::string label;
    std::string icon;     /* nom d'icône de thème (ou chemin) ou "" */
    std::string cmd;      /* commande d'action ou "" */
    bool separator = false;
    bool has_check = false;
    bool check_val = false;
};

struct SermoMenu {        /* un <menu> = un top-level de la barre */
    std::string label;
    std::vector<SermoMenuItem> items;
};

struct SermoMenuCarrier { /* posé en user_data() du widget poussé */
    int kind;             /* 0 = menuitem, 1 = menu */
    SermoMenuItem item;
    SermoMenu     menu;
};
#endif
#endif
