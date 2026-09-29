/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * printing.c — messages de diagnostic (implémentation indépendante)
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */

/* isatty()/fileno() sont POSIX : -std=c11 seul ne les expose pas. */
#define _POSIX_C_SOURCE 200809L

#include "gtk3d.h"
#include "printing.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

/* Couleurs ANSI, utilisées seulement si stderr est un terminal interactif. */
#define PIP_ANSI_RESET   "\x1b[0m"
#define PIP_ANSI_DEBUG   "\x1b[32m"   /* vert : diagnostic */
#define PIP_ANSI_WARNING "\x1b[33m"   /* jaune : avertissement */

/*
 * Coeur commun aux deux messages de diagnostic. N'écrit jamais sur stdout :
 * ce canal transporte les lignes NOM="valeur" du programme et ne doit
 * jamais recevoir un message de mise au point.
 */
static void
pip_message_print_common(const char *ansi_color, const gchar *function,
                          const gchar *format, va_list args)
{
    const gchar *safe_function = (function != NULL) ? function : "(fonction inconnue)";
    const gchar *safe_format = (format != NULL) ? format : "(message absent)";
    int colorize = isatty(fileno(stderr));

    if (colorize) {
        fprintf(stderr, "%s", ansi_color);
    }

    fprintf(stderr, "[%s] ", safe_function);

    if (format != NULL) {
        vfprintf(stderr, safe_format, args);
    } else {
        fprintf(stderr, "%s", safe_format);
    }

    if (colorize) {
        fprintf(stderr, "%s", PIP_ANSI_RESET);
    }

    fprintf(stderr, "\n");
    fflush(stderr);
}

void
pip_message_print_debug(const gchar *function, const gchar *format, ...)
{
    va_list args;

    va_start(args, format);
    pip_message_print_common(PIP_ANSI_DEBUG, function, format, args);
    va_end(args);
}

void
pip_message_print_warning(const gchar *function, const gchar *format, ...)
{
    va_list args;

    va_start(args, format);
    pip_message_print_common(PIP_ANSI_WARNING, function, format, args);
    va_end(args);
}

void
unimplemented(const char *function_name, const char *filename, gint linenumber)
{
    const char *safe_function = (function_name != NULL) ? function_name : "(fonction inconnue)";
    const char *safe_filename = (filename != NULL) ? filename : "(fichier inconnu)";
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "La fonctionnalité « %s » n'est pas prise en charge "
                                     "par ce backend.\n(%s, ligne %d)",
                                     safe_function, safe_filename, linenumber);

    if (dialog == NULL) {
        return;
    }

    gtk_dialog_run(dialog);
    gtk_widget_destroy(dialog);
}
