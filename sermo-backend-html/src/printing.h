/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * printing.h — messages de diagnostic (implémentation indépendante)
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef PRINTING_H
#define PRINTING_H

void pip_message_print_debug(const gchar *function, const gchar *format, ...)
    __attribute__((format(printf, 2, 3)));

void pip_message_print_warning(const gchar *function, const gchar *format, ...)
    __attribute__((format(printf, 2, 3)));

void unimplemented(const char *function_name, const char *filename, gint linenumber);

#endif
