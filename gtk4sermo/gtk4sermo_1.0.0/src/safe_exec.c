/*
 * safe_exec.c: Secure command execution wrappers.
 * Replaces direct calls to system() and popen() with GLib-based equivalents.
 *
 * Design notes:
 *
 * 1. METACHARACTERS: If the command string contains shell metacharacters
 *    (|, &, ;, <, >, (, ), {, }, $, `, \, ", ', ~, *) — as many gtk4sermo
 *    <input> commands do — g_shell_parse_argv() cannot tokenise them.
 *    In that case both safe_system() and safe_popen() fall back to
 *    /bin/sh -c, which preserves full shell functionality at the cost of
 *    re-enabling injection risk for that command.  The fall-back is logged
 *    via g_debug() so callers can audit it.
 *
 * 2. FCLOSE vs PCLOSE: safe_popen() returns FILE* via fdopen(), NOT via
 *    popen().  Callers MUST use fclose(), never pclose() — pclose() on an
 *    fdopen() FILE* is undefined behaviour.
 *
 * 3. ZOMBIE REAPING: G_SPAWN_DO_NOT_REAP_CHILD is used so we can attach
 *    a GChildWatch to reap the child automatically when the pipe is drained.
 *    This prevents zombie accumulation on timer/progressbar widgets.
 *
 * gtk4sermo-1.0.0 — Haplo-Linux (devel@haplo-dialog.fr), 2026
 * License: GPL-2.0-or-later
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include "safe_exec.h"

/* Shell metacharacters that require /bin/sh -c fallback. */
#define SHELL_METACHARACTERS "|&;<>(){}$`\\\"'~*?"

/*
 * _has_shell_metacharacters:
 * Returns TRUE if the command contains any character that g_shell_parse_argv()
 * cannot handle and that requires a /bin/sh -c invocation.
 */
static gboolean _has_shell_metacharacters(const gchar *command)
{
    return strpbrk(command, SHELL_METACHARACTERS) != NULL;
}

/*
 * _reap_child_cb:
 * GChildWatchFunc: called by the GLib main loop when a child process exits.
 * Closes the GPid handle to release the zombie.
 */
static void _reap_child_cb(GPid pid, gint status, gpointer user_data)
{
    (void)status;
    (void)user_data;
    g_spawn_close_pid(pid);
}

/*
 * safe_system:
 * Execute a shell command safely.  Uses g_spawn_sync() with direct exec()
 * when no metacharacters are present; falls back to /bin/sh -c otherwise.
 * Returns the exit status, or -1 on error.
 */
gint safe_system(const gchar *command)
{
    gchar  **argv        = NULL;
    GError  *error       = NULL;
    gint     exit_status = -1;
    gboolean use_shell;

    if (!command || *command == '\0') {
        g_warning("safe_system: empty command");
        return -1;
    }
    if (strlen(command) > 65535) {
        g_warning("safe_system: command too long (%zu bytes)", strlen(command));
        return -1;
    }

    use_shell = _has_shell_metacharacters(command);

    if (use_shell) {
        g_warning("safe_system: shell fallback (injection risk) for '%s'", command);
        if (g_getenv("HAPLO_NO_SHELL_FALLBACK")) {
            g_critical("safe_system: shell fallback refused (HAPLO_NO_SHELL_FALLBACK set)");
            return -1;
        }
        argv = g_new(gchar *, 4);
        argv[0] = g_strdup("/bin/sh");
        argv[1] = g_strdup("-c");
        argv[2] = g_strdup(command);
        argv[3] = NULL;
    } else {
        if (!g_shell_parse_argv(command, NULL, &argv, &error)) {
            g_warning("safe_system: cannot parse '%s': %s",
                      command, error->message);
            g_error_free(error);
            return -1;
        }
    }

    if (!g_spawn_sync(NULL, argv, NULL,
                      G_SPAWN_SEARCH_PATH,
                      NULL, NULL,
                      NULL, NULL,
                      &exit_status,
                      &error)) {
        g_warning("safe_system: spawn failed for '%s': %s",
                  command, error->message);
        g_error_free(error);
        g_strfreev(argv);
        return -1;
    }

    g_strfreev(argv);
    return exit_status;
}

/*
 * safe_popen:
 * Open a read pipe to a command.  Returns FILE* (via fdopen) that the caller
 * closes with fclose() — NOT pclose().
 *
 * Uses g_spawn_async_with_pipes() with direct exec() when possible; falls
 * back to /bin/sh -c when the command contains shell metacharacters.
 * The child is reaped automatically via g_child_watch_add() to prevent
 * zombie accumulation.
 */
FILE *safe_popen(const gchar *command)
{
    gchar   **argv      = NULL;
    GError   *error     = NULL;
    gint      stdout_fd = -1;
    GPid      child_pid;
    FILE     *stream;
    gboolean  use_shell;

    if (!command || *command == '\0') {
        g_warning("safe_popen: empty command");
        return NULL;
    }
    if (strlen(command) > 65535) {
        g_warning("safe_popen: command too long (%zu bytes)", strlen(command));
        return NULL;
    }

    use_shell = _has_shell_metacharacters(command);

    if (use_shell) {
        g_warning("safe_popen: shell fallback (injection risk) for '%s'", command);
        if (g_getenv("HAPLO_NO_SHELL_FALLBACK")) {
            g_critical("safe_popen: shell fallback refused (HAPLO_NO_SHELL_FALLBACK set)");
            return NULL;
        }
        argv = g_new(gchar *, 4);
        argv[0] = g_strdup("/bin/sh");
        argv[1] = g_strdup("-c");
        argv[2] = g_strdup(command);
        argv[3] = NULL;
    } else {
        if (!g_shell_parse_argv(command, NULL, &argv, &error)) {
            g_warning("safe_popen: cannot parse '%s': %s",
                      command, error->message);
            g_error_free(error);
            return NULL;
        }
    }

    if (!g_spawn_async_with_pipes(
                NULL, argv, NULL,
                G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                NULL, NULL,
                &child_pid,
                NULL,         /* stdin  — not needed */
                &stdout_fd,   /* stdout — we read from here */
                NULL,         /* stderr — inherit */
                &error)) {
        g_warning("safe_popen: spawn failed for '%s': %s",
                  command, error->message);
        g_error_free(error);
        g_strfreev(argv);
        return NULL;
    }

    g_strfreev(argv);

    /* Register a child watcher so the GLib main loop reaps the zombie
     * automatically once the child exits, without any additional caller
     * action required. */
    g_child_watch_add(child_pid, _reap_child_cb, NULL);

    /* Wrap the raw fd in a FILE* — caller uses fgets()/fclose(). */
    stream = fdopen(stdout_fd, "r");
    if (!stream) {
        g_warning("safe_popen: fdopen failed for '%s'", command);
        close(stdout_fd);
        /* Child watcher will still reap the child correctly. */
        return NULL;
    }

    return stream;
}
