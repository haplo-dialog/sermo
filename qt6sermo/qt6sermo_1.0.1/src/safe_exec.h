/*
 * safe_exec.h: Secure command execution wrappers.
 * gtk3d-1.0.0 — haplo-dialog (devel@haplo-dialog.fr), 2026 — License: GPL-2.0-or-later
 */
#ifndef SAFE_EXEC_H
#define SAFE_EXEC_H

#include <stdio.h>
#ifndef QT6_COMPAT_H
#include <glib.h>
#endif

/*
 * Hardening: when a command contains shell metacharacters, these wrappers
 * fall back to /bin/sh -c (logged via g_warning). Set the environment
 * variable HAPLO_NO_SHELL_FALLBACK to refuse that fallback entirely
 * (safe_system returns -1, safe_popen returns NULL) — use it when commands
 * may incorporate untrusted input.
 */
#ifdef __cplusplus
extern "C" {
#endif

/* Replaces system(): direct exec() by argv when the command carries no shell
 * metacharacter, logged /bin/sh -c fallback otherwise (refusable with
 * HAPLO_NO_SHELL_FALLBACK=1, and refused outright while HAPLO_ALLOWED_CMDS
 * is set). NOT a shell-free guarantee. */
gint  safe_system(const gchar *command);

/* Drop-in replacement for popen(cmd,"r"): returns FILE*, close with fclose(). */
FILE *safe_popen (const gchar *command);

#ifdef __cplusplus
}
#endif

#endif /* SAFE_EXEC_H */
