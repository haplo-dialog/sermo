/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * safe_exec.h: Secure command execution wrappers.
 * gtk3sermo-1.0.0 — Haplo-Linux (devel@haplo-dialog.fr), 2026 — License: GPL-2.0-or-later
 */
#ifndef SAFE_EXEC_H
#define SAFE_EXEC_H

#include <stdio.h>
#include <glib.h>

/*
 * Hardening: when a command contains shell metacharacters, these wrappers
 * fall back to /bin/sh -c (logged via g_warning). Set the environment
 * variable HAPLO_NO_SHELL_FALLBACK to refuse that fallback entirely
 * (safe_system returns -1, safe_popen returns NULL) — use it when commands
 * may incorporate untrusted input.
 */
/* Drop-in replacement for system(): no /bin/sh -c, no shell injection. */
gint  safe_system(const gchar *command);

/* Drop-in replacement for popen(cmd,"r"): returns FILE*, close with fclose(). */
FILE *safe_popen (const gchar *command);

#endif /* SAFE_EXEC_H */
