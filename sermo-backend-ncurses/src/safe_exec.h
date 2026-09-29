/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * safe_exec.h: Secure command execution wrappers.
 * gtk3d-1.0.0 — haplo-dialog (devel@haplo-dialog.fr), 2026 — License: GPL-2.0-or-later
 */
#ifndef SAFE_EXEC_H
#define SAFE_EXEC_H

#include <stdio.h>
#ifndef SDL3_COMPAT_H        /* port SDL3 : gint/gchar viennent de ncurses-compat.h */
#include <glib.h>
#endif

/*
 * Hardening: when a command contains shell metacharacters, these wrappers
 * fall back to /bin/sh -c (logged via g_warning). Set the environment
 * variable SERMO_NO_SHELL_FALLBACK to refuse that fallback entirely
 * (safe_system returns -1, safe_popen returns NULL) — use it when commands
 * may incorporate untrusted input.
 */
/* Replaces system(): direct exec() by argv when the command carries no shell
 * metacharacter, logged /bin/sh -c fallback otherwise (refusable with
 * SERMO_NO_SHELL_FALLBACK=1, and refused outright while SERMO_ALLOWED_CMDS
 * is set). NOT a shell-free guarantee. */
gint  safe_system(const gchar *command);

/* Drop-in replacement for popen(cmd,"r"): returns FILE*, close with fclose(). */
FILE *safe_popen (const gchar *command);

#endif /* SAFE_EXEC_H */
