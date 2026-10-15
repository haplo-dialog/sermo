/*
 * sermo_lanceur.h — le lanceur de commandes de sermo, en un seul endroit.
 *
 * Avant lui, chaque shim (cœur neutre, port ncurses…) portait sa copie de
 * fork/exec/pipe. Le cœur neutre et le port ncurses passent désormais par
 * ici ; c'est aussi le seul fichier à changer pour un système sans fork.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef SERMO_LANCEUR_H
#define SERMO_LANCEUR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Le shell du repli « sh -c » de safe_system/safe_popen.
 * SERMO_SHELL s'il désigne, par un chemin absolu, un programme exécutable ;
 * sinon le shell choisi à la construction (SERMO_SHELL_DEFAUT, « /bin/sh »
 * par défaut). Une valeur refusée est signalée une fois sur stderr. */
const char *sermo_shell(void);

/* L'option qui passe une commande à ce shell : « -c », ou « /C » pour
 * cmd.exe sous Windows. */
const char *sermo_shell_option(void);

/* Lance argv[0] (cherché dans le PATH) et attend sa fin.
 * envp NULL : l'environnement du processus.
 * Rend 0 si le programme a tourné (*statut = statut brut de waitpid), -1 s'il
 * n'a pas pu être lancé (*erreur = message alloué, à libérer par free). */
int sermo_lance_sync(char *const argv[], char *const envp[],
                     int *statut, char **erreur);

/* Lance argv[0] sans l'attendre. L'enfant est détaché (double fork : init le
 * récupère, aucun zombie). Si fd_sortie n'est pas NULL, sa sortie standard
 * arrive par un tube dont on reçoit le bout lecteur. *pid reçoit le pid réel.
 * Rend 0 si le programme est lancé, -1 sinon (*erreur comme ci-dessus). */
int sermo_lance_async(char *const argv[], char *const envp[],
                      int *fd_sortie, int *pid, char **erreur);

#ifdef __cplusplus
}
#endif

#endif /* SERMO_LANCEUR_H */
