/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * printing.c — messages de diagnostic (implémentation indépendante)
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#define _POSIX_C_SOURCE 200809L

#include "gtkdialog.h"
#include "printing.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#define PRINTING_COLOR_DEBUG   "\033[32m"
#define PRINTING_COLOR_WARNING "\033[33m"
#define PRINTING_COLOR_RESET   "\033[0m"

Sdl3ObjData *sdl3_objdata_head = NULL;

static void
pip_message_print_generic(const gchar *color, const gchar *function,
                           const gchar *format, va_list args)
{
    int is_tty;

    if (function == NULL) {
        function = "(inconnu)";
    }
    if (format == NULL) {
        format = "(inconnu)";
    }

    is_tty = isatty(fileno(stderr));

    if (is_tty) {
        fprintf(stderr, "%s", color);
    }

    fprintf(stderr, "%s: ", function);
    vfprintf(stderr, format, args);

    if (is_tty) {
        fprintf(stderr, "%s", PRINTING_COLOR_RESET);
    }

    fprintf(stderr, "\n");
    fflush(stderr);
}

void
pip_message_print_debug(const gchar *function, const gchar *format, ...)
{
    va_list args;

    va_start(args, format);
    pip_message_print_generic(PRINTING_COLOR_DEBUG, function, format, args);
    va_end(args);
}

void
pip_message_print_warning(const gchar *function, const gchar *format, ...)
{
    va_list args;

    va_start(args, format);
    pip_message_print_generic(PRINTING_COLOR_WARNING, function, format, args);
    va_end(args);
}

void
unimplemented(const char *function_name, const char *filename, gint linenumber)
{
    if (function_name == NULL) {
        function_name = "(inconnu)";
    }
    if (filename == NULL) {
        filename = "(inconnu)";
    }

    fprintf(stderr,
            "fonctionnalite non prise en charge par ce backend : %s (%s:%d)\n",
            function_name, filename, (int) linenumber);
}
