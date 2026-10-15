/*
 * sermo_lanceur.c — le lanceur de commandes de sermo (voir sermo_lanceur.h).
 *
 * Le code POSIX vient du shim du cœur neutre, inchangé : un tube O_CLOEXEC
 * que l'enfant n'écrit QUE si exec échoue (fermé tout seul, donc EOF côté
 * parent, si exec réussit), et le double fork du lancement asynchrone, dont
 * le vrai pid du petit-fils remonte par le même tube.
 *
 * Écrit en C simple, sans GLib : il sert aussi aux variantes GLib et GTK 4 du
 * cœur (sermo_shell).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE   /* pipe2, execvpe */
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef _WIN32
#include <fcntl.h>
#include <sys/wait.h>
#endif
#include "sermo_lanceur.h"
#ifdef _WIN32
#include "sermo_win32.h"
#endif

#ifndef _WIN32
/* Tube fermé à l'exec : pipe2 là où il existe ; sinon (macOS) pipe puis
 * FD_CLOEXEC — sans course ici, le lancement se fait d'un seul fil. */
static int tube_cloexec(int fd[2])
{
#ifdef SERMO_SANS_PIPE2
	if (pipe(fd) != 0)
		return -1;
	fcntl(fd[0], F_SETFD, FD_CLOEXEC);
	fcntl(fd[1], F_SETFD, FD_CLOEXEC);
	return 0;
#else
	return pipe2(fd, O_CLOEXEC);
#endif
}

/* exec avec un environnement choisi : execvpe là où il existe ; sinon (macOS)
 * l'enfant, déjà séparé du parent, remplace son environ puis appelle execvp. */
static void executer(char *const argv[], char *const envp[])
{
#ifdef SERMO_SANS_EXECVPE
	if (envp) {
		extern char **environ;
		environ = (char **) envp;
	}
	execvp(argv[0], argv);
#else
	if (envp) execvpe(argv[0], argv, envp); else execvp(argv[0], argv);
#endif
}
#endif

#ifndef SERMO_SHELL_DEFAUT
#define SERMO_SHELL_DEFAUT "/bin/sh"
#endif

/* Le shell sans SERMO_SHELL. Windows : celui du système (%ComSpec%, sinon
 * cmd.exe de System32) ; ailleurs, celui choisi à la construction. */
static const char *shell_defaut(void)
{
#ifdef _WIN32
	const char *c = getenv("ComSpec");

	return (c != NULL && *c != '\0') ? c : "C:\\Windows\\System32\\cmd.exe";
#else
	return SERMO_SHELL_DEFAUT;
#endif
}

static int executable(const char *chemin)
{
#ifdef _WIN32
	return access(chemin, 0) == 0;   /* le CRT de Windows refuse X_OK */
#else
	return access(chemin, X_OK) == 0;
#endif
}

#ifndef _WIN32
static void poser_erreur(char **erreur, const char *msg)
{
	if (erreur)
		*erreur = strdup(msg ? msg : "erreur");
}
#endif

static int est_absolu(const char *chemin)
{
#ifdef _WIN32
	/* « C:\… », « C:/… » ou chemin réseau « \\serveur\… » */
	if (((chemin[0] >= 'A' && chemin[0] <= 'Z') || (chemin[0] >= 'a' && chemin[0] <= 'z'))
	    && chemin[1] == ':' && (chemin[2] == '\\' || chemin[2] == '/'))
		return 1;
	return chemin[0] == '\\' && chemin[1] == '\\';
#else
	return chemin[0] == '/';
#endif
}

const char *sermo_shell(void)
{
	static int deja_signale = 0;
	const char *s = getenv("SERMO_SHELL");

	if (s == NULL || *s == '\0')
		return shell_defaut();
	if (est_absolu(s) && executable(s))
		return s;
	/* Refus signalé une fois : un shell relatif serait cherché dans le PATH,
	 * un fichier non exécutable ferait échouer chaque commande en silence. */
	if (!deja_signale) {
		deja_signale = 1;
		fprintf(stderr, "sermo: SERMO_SHELL ignorée (« %s ») : il faut le chemin "
		        "absolu d'un programme exécutable ; %s est utilisé.\n",
		        s, shell_defaut());
	}
	return shell_defaut();
}

const char *sermo_shell_option(void)
{
#ifdef _WIN32
	if (sermo_win32_est_cmd(sermo_shell()))
		return "/C";
#endif
	return "-c";
}

#ifdef _WIN32

/* Windows : pas de fork ; CreateProcess, dans sermo_win32.c. */
int sermo_lance_sync(char *const argv[], char *const envp[],
                     int *statut, char **erreur)
{
	return sermo_win32_lance(argv, envp, 1, statut, NULL, NULL, erreur);
}

int sermo_lance_async(char *const argv[], char *const envp[],
                      int *fd_sortie, int *pid, char **erreur)
{
	return sermo_win32_lance(argv, envp, 0, NULL, fd_sortie, pid, erreur);
}

#else

int sermo_lance_sync(char *const argv[], char *const envp[],
                     int *statut, char **erreur)
{
	int ep[2];
	pid_t pid;
	int execerr = 0, st = 0;
	ssize_t r;

	if (!argv || !argv[0]) { poser_erreur(erreur, "no argv"); return -1; }
	/* Commande introuvable ⇒ échec RAPPORTÉ : l'enfant n'écrit son errno dans
	 * ce tube que si exec échoue. */
	if (tube_cloexec(ep) != 0) { poser_erreur(erreur, "pipe failed"); return -1; }
	pid = fork();
	if (pid < 0) {
		close(ep[0]); close(ep[1]);
		poser_erreur(erreur, "fork failed");
		return -1;
	}
	if (pid == 0) {
		close(ep[0]);
		executer(argv, envp);
		int err = errno;
		ssize_t w = write(ep[1], &err, sizeof err); (void) w;
		_exit(127);
	}
	close(ep[1]);
	while ((r = read(ep[0], &execerr, sizeof execerr)) < 0 && errno == EINTR) { }
	close(ep[0]);
	while (waitpid(pid, &st, 0) < 0 && errno == EINTR) { }
	if (r > 0) { poser_erreur(erreur, strerror(execerr)); return -1; }
	if (statut) *statut = st;
	return 0;
}

int sermo_lance_async(char *const argv[], char *const envp[],
                      int *fd_sortie, int *pid, char **erreur)
{
	int sortie[2] = { -1, -1 };
	int ep[2];
	pid_t inter;
	int intermediaire = 0;
	pid_t vrai_pid = -1;
	int execerr = 0;
	ssize_t r, r2;

	if (!argv || !argv[0]) { poser_erreur(erreur, "no argv"); return -1; }
	if (fd_sortie && pipe(sortie) != 0) { poser_erreur(erreur, "pipe failed"); return -1; }
	if (tube_cloexec(ep) != 0) {
		if (fd_sortie) { close(sortie[0]); close(sortie[1]); }
		poser_erreur(erreur, "pipe failed");
		return -1;
	}
	inter = fork();
	if (inter < 0) {
		if (fd_sortie) { close(sortie[0]); close(sortie[1]); }
		close(ep[0]); close(ep[1]);
		poser_erreur(erreur, "fork failed");
		return -1;
	}
	if (inter == 0) {
		close(ep[0]);
		pid_t petit = fork();
		if (petit == 0) {
			if (fd_sortie) {
				dup2(sortie[1], STDOUT_FILENO);
				close(sortie[0]); close(sortie[1]);
			}
			executer(argv, envp);
			int err = errno;
			ssize_t w = write(ep[1], &err, sizeof err); (void) w;
			_exit(127);
		}
		/* pid du petit-fils vers le parent, par un canal qui survit à _exit */
		ssize_t w = write(ep[1], &petit, sizeof petit); (void) w;
		_exit(0);
	}
	close(ep[1]);
	if (fd_sortie) { close(sortie[1]); *fd_sortie = sortie[0]; }
	while (waitpid(inter, &intermediaire, 0) < 0 && errno == EINTR) { }
	while ((r = read(ep[0], &vrai_pid, sizeof vrai_pid)) < 0 && errno == EINTR) { }
	while ((r2 = read(ep[0], &execerr, sizeof execerr)) < 0 && errno == EINTR) { }
	close(ep[0]);
	if (r2 > 0) {   /* le petit-fils a écrit son errno : exec a échoué */
		if (fd_sortie) { close(*fd_sortie); *fd_sortie = -1; }
		poser_erreur(erreur, strerror(execerr));
		return -1;
	}
	if (pid) *pid = (int) (r > 0 ? vrai_pid : inter);
	return 0;
}

#endif /* _WIN32 */
