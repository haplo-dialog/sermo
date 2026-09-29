/* SPDX-License-Identifier: GPL-2.0-or-later */
/* sermo_backend_gtk3.c — hook d'init toolkit du backend gtk3. */
#include <gtk/gtk.h>
#include "sermo-contract.h"   /* contrat cœur <-> backend (MIT), coexiste avec la vraie GLib */
void sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)
{
    /* SERMO_DARK pilote la variante Adwaita (cf. interrupteur unique des six ports). */
    if (!print_ir) {
        const char *sd = getenv("SERMO_DARK");
        if (sd && *sd && !getenv("GTK_THEME"))
            setenv("GTK_THEME", sd[0] != '0' ? "Adwaita:dark" : "Adwaita", 1);
        gtk_init(argc, argv);
    } else {
        gtk_init_check(argc, argv);   /* headless : non fatal */
    }
}

/* Globales de visibilité : la lib les déclare extern ; le backend les définit
 * (comme il fournit widget_*_create). */
GList *widget_show_list = NULL;
GList *widget_hide_list = NULL;
