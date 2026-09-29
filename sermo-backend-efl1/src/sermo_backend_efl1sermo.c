/* sermo_backend_efl1sermo.c — hook d'init toolkit du backend efl1.
 * Le cœur (libsermocore) appelle sermo_backend_toolkit_init() à la place de
 * gtk_init. Côté EFL, efl-compat.h mappe gtk_init → elm_init ; on initialise
 * Elementary ici. elm_init n'ouvre AUCUN affichage (la fenêtre est créée à la
 * demande par efl_main_win_get()), donc l'appel reste sûr en --print-ir. */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#include "efl-compat.h"
#include "sermo-contract.h"   /* contrat cœur <-> backend (MIT) */
#include <stdlib.h>               /* setenv */

/* --render-png FILE : le cœur (gtkdialog.c) parse l'option AVANT cet appel. */
extern char *option_render_png;

void sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)
{
    (void) print_ir;              /* elm_init n'ouvre pas d'affichage ; fenêtre créée à la demande */
    /* Rendu offscreen déterministe : forcer le moteur ecore_evas « buffer »
     * AVANT elm_init. La fenêtre (elm_win) naît alors sur un canvas mémoire —
     * aucun serveur X/Wayland requis, l'IA « voit » le dialogue sans capture
     * externe fragile. On ne touche l'environnement que si l'utilisateur ne
     * l'a pas déjà fixé (ELM_ENGINE / ELM_DISPLAY gardent la main). */
    if (option_render_png && *option_render_png) {
        if (!getenv("ELM_ENGINE") && !getenv("ELM_DISPLAY"))
            setenv("ELM_ENGINE", "buffer", 1);
    }
    elm_init(*argc, *argv);       /* = gtk_init(argc, argv) via le shim efl-compat.h */
}

/* Globales de visibilité : le cœur les déclare extern (gtk3d.h) ; le backend
 * les définit, comme il fournit widget_*_create. GList vient de la vraie GLib
 * (HAVE_GLIB=1), ABI identique au GList du cœur neutre {data,next,prev}. */
GList *widget_show_list = NULL;
GList *widget_hide_list = NULL;
