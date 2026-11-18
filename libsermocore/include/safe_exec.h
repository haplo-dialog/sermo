/* SPDX-License-Identifier: GPL-2.0-or-later */
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
 * variable SERMO_NO_SHELL_FALLBACK to refuse that fallback entirely
 * (safe_system returns -1, safe_popen returns NULL) — use it when commands
 * may incorporate untrusted input.
 */
#ifdef __cplusplus
extern "C" {
#endif

/* Replaces system(): direct exec() by argv when the command carries no shell
 * metacharacter, logged /bin/sh -c fallback otherwise (refusable with
 * SERMO_NO_SHELL_FALLBACK=1, and refused outright while SERMO_ALLOWED_CMDS
 * is set). NOT a shell-free guarantee. */
gint  safe_system(const gchar *command);

/* Drop-in replacement for popen(cmd,"r"): returns FILE*, close with fclose(). */
FILE *safe_popen (const gchar *command);

/* Refuse désormais toute commande, comme une liste blanche qu'aucune commande
 * ne peut satisfaire. Appelé par --render-png (sauf SERMO_RENDER_EXEC=1). */
void safe_exec_refuser_tout(void);

/* Vrai après safe_exec_refuser_tout(). À consulter par tout code qui lance un
 * programme sans passer par safe_system/safe_popen (<terminal> via VTE). */
gboolean safe_exec_tout_refuse(void);

/* Pose la variable d'un widget dans l'environnement — sauf si son nom est
 * RÉSERVÉ : SERMO_*, HAPLO_*, LD_*, PATH, IFS, ENV, BASH_ENV, SHELLOPTS, PS4.
 * Un <variable>SERMO_ALLOWED_CMDS</variable> remplaçait la liste blanche par
 * la valeur du widget, un <variable>PATH</variable> déplaçait la recherche
 * des commandes. Le refus est dit une fois par nom ; la variable reste
 * imprimée à la sortie comme les autres. Rend 1 si posée, 0 si refusée. */
int sermo_exporter_variable(const char *nom, const char *valeur, int ecraser);

#ifdef __cplusplus
}
#endif

#endif /* SAFE_EXEC_H */
