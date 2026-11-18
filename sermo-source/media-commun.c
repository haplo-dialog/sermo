/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * media-commun.c — voir media-commun.h.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "media-commun.h"

int
media_nom(const char *programme, const char *base, char *nom, size_t taille)
{
	static const char prefixe[] = "media://";
	if (!base || strncmp(base, prefixe, sizeof prefixe - 1) != 0) {
		fprintf(stderr, "%s: adresse refusée : elle doit commencer par media://NOM\n", programme);
		return -1;
	}
	const char *n = base + sizeof prefixe - 1;
	size_t l = strlen(n);
	if (l == 0 || l > MEDIA_NOM_MAX || l >= taille) {
		fprintf(stderr, "%s: nom de lecteur vide ou trop long (32 caractères au plus)\n", programme);
		return -1;
	}
	for (size_t i = 0; i < l; i++) {
		char c = n[i];
		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
			fprintf(stderr, "%s: nom de lecteur « %s » refusé (lettres, chiffres, _ et - seulement)\n", programme, n);
			return -1;
		}
	}
	memcpy(nom, n, l + 1);
	return 0;
}

/* Le dossier existe, n'est pas un lien, est à l'utilisateur et en 700. */
static int
dossier_sur(const char *programme, const char *chemin)
{
	struct stat st;
	if (lstat(chemin, &st) != 0 || !S_ISDIR(st.st_mode) || st.st_uid != geteuid() || (st.st_mode & 077) != 0) {
		fprintf(stderr, "%s: %s n'est pas un dossier à vous en mode 700 : refusé\n", programme, chemin);
		return -1;
	}
	return 0;
}

int
media_racine(const char *programme, char *chemin, size_t taille, int creer)
{
	const char *run = getenv("XDG_RUNTIME_DIR");
	if (!run || run[0] != '/') {
		fprintf(stderr, "%s: XDG_RUNTIME_DIR absent ou relatif : pas de dossier privé pour le lecteur\n", programme);
		return -1;
	}
	int n = snprintf(chemin, taille, "%s/sermo-media", run);
	if (n < 0 || (size_t)n >= taille)
		return -1;
	if (creer && mkdir(chemin, 0700) != 0 && errno != EEXIST) {
		fprintf(stderr, "%s: impossible de créer %s (%s)\n", programme, chemin, strerror(errno));
		return -1;
	}
	if (!creer && access(chemin, F_OK) != 0)
		return -1;
	return dossier_sur(programme, chemin);
}

int
media_dossier(const char *programme, const char *nom, char *chemin, size_t taille, int creer)
{
	char racine[512];
	if (media_racine(programme, racine, sizeof racine, creer) != 0)
		return -1;
	int n = snprintf(chemin, taille, "%s/%s", racine, nom);
	if (n < 0 || (size_t)n >= taille)
		return -1;
	if (creer && mkdir(chemin, 0700) != 0 && errno != EEXIST) {
		fprintf(stderr, "%s: impossible de créer %s (%s)\n", programme, chemin, strerror(errno));
		return -1;
	}
	if (!creer && access(chemin, F_OK) != 0)
		return -1;
	return dossier_sur(programme, chemin);
}

pid_t
media_pid_vivant(const char *dossier, const char *fichier)
{
	char chemin[600], tampon[32];
	snprintf(chemin, sizeof chemin, "%s/%s", dossier, fichier);
	FILE *f = fopen(chemin, "r");
	if (!f)
		return 0;
	size_t n = fread(tampon, 1, sizeof tampon - 1, f);
	fclose(f);
	tampon[n] = '\0';
	char *fin;
	long pid = strtol(tampon, &fin, 10);
	if (pid <= 1 || (*fin != '\0' && *fin != '\n'))
		return 0;
	if (kill((pid_t)pid, 0) != 0)
		return 0;
#ifdef __linux__
	/* Le numéro peut avoir été REPRIS par un autre processus après la mort du
	 * nôtre (fichier resté d'un arrêt brutal) : l'appelant lui enverrait
	 * SIGTERM. Un processus né APRÈS l'écriture du fichier n'est pas celui que
	 * le fichier désigne. Si /proc ne se lit pas, on garde l'ancienne réponse. */
	{
		struct stat st;
		char proc[64], ligne[1024];
		double demarre = -1, amorce = -1;
		long tics = sysconf(_SC_CLK_TCK);
		snprintf(proc, sizeof proc, "/proc/%ld/stat", pid);
		FILE *p = fopen(proc, "r");
		if (p) {
			if (fgets(ligne, sizeof ligne, p)) {
				/* champ 22 (starttime), compté après la « ) » qui ferme le nom */
				char *q = strrchr(ligne, ')');
				unsigned long long t = 0;
				if (q && sscanf(q + 1, " %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %*d %llu",
				                &t) == 1 && tics > 0)
					demarre = (double)t / (double)tics;
			}
			fclose(p);
		}
		p = fopen("/proc/stat", "r");
		if (p) {
			while (fgets(ligne, sizeof ligne, p))
				if (sscanf(ligne, "btime %lf", &amorce) == 1)
					break;
			fclose(p);
		}
		if (demarre >= 0 && amorce > 0 && stat(chemin, &st) == 0
		    && amorce + demarre > (double)st.st_mtime + 2.0)
			return 0;
	}
#endif
	return (pid_t)pid;
}

int
media_ipc(const char *dossier, const char *requete, char *reponse, size_t taille)
{
	char chemin[600];
	struct stat st;
	snprintf(chemin, sizeof chemin, "%s/ipc", dossier);
	if (lstat(chemin, &st) != 0 || !S_ISSOCK(st.st_mode) || st.st_uid != geteuid() || (st.st_mode & 077) != 0)
		return -1;
	struct sockaddr_un adr = { .sun_family = AF_UNIX };
	if (strlen(chemin) >= sizeof adr.sun_path)
		return -1;
	snprintf(adr.sun_path, sizeof adr.sun_path, "%s", chemin);
	int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0)
		return -1;
	if (connect(fd, (struct sockaddr *)&adr, sizeof adr) != 0) {
		close(fd);
		return -1;
	}
	size_t l = strlen(requete);
	/* MSG_NOSIGNAL : si le lecteur ferme son socket entre les deux envois,
	 * write() tuait ce programme (SIGPIPE, code 141) au lieu de rendre
	 * l'erreur — mesuré 1 fois sur 200 contre un socket qui ferme aussitôt. */
	if (send(fd, requete, l, MSG_NOSIGNAL) != (ssize_t)l || send(fd, "\n", 1, MSG_NOSIGNAL) != 1) {
		close(fd);
		return -1;
	}
	/* Lire jusqu'à une ligne qui porte « "error": » (la réponse) ; 2 s au plus. */
	char tampon[8192];
	size_t plein = 0;
	struct timespec debut, maintenant;
	clock_gettime(CLOCK_MONOTONIC, &debut);
	for (;;) {
		clock_gettime(CLOCK_MONOTONIC, &maintenant);
		long ecoule = (maintenant.tv_sec - debut.tv_sec) * 1000 + (maintenant.tv_nsec - debut.tv_nsec) / 1000000;
		if (ecoule >= 2000)
			break;
		struct pollfd p = { .fd = fd, .events = POLLIN };
		if (poll(&p, 1, (int)(2000 - ecoule)) <= 0)
			break;
		ssize_t r = read(fd, tampon + plein, sizeof tampon - 1 - plein);
		if (r <= 0)
			break;
		plein += (size_t)r;
		tampon[plein] = '\0';
		char *ligne = tampon, *nl;
		while ((nl = strchr(ligne, '\n'))) {
			*nl = '\0';
			if (strstr(ligne, "\"error\":")) {
				snprintf(reponse, taille, "%s", ligne);
				close(fd);
				return 0;
			}
			ligne = nl + 1;
		}
		/* Garder le morceau de ligne inachevé. */
		size_t reste = plein - (size_t)(ligne - tampon);
		memmove(tampon, ligne, reste);
		plein = reste;
		if (plein >= sizeof tampon - 1)
			plein = 0;                /* ligne démesurée : jetée */
	}
	close(fd);
	return -1;
}

const char *
media_valeur(const char *programme, const char *brute)
{
	if (!brute || brute[0] != '$')
		return brute;
	const char *nom = brute + 1;
	size_t i = 0;
	for (; nom[i]; i++)
		if (!((nom[i] >= 'A' && nom[i] <= 'Z') || (nom[i] >= 'a' && nom[i] <= 'z') || nom[i] == '_'
		      || (i > 0 && nom[i] >= '0' && nom[i] <= '9')))
			break;
	if (i == 0 || nom[i] != '\0') {
		fprintf(stderr, "%s: valeur=%s : nom de variable invalide\n", programme, brute);
		return NULL;
	}
	const char *v = getenv(nom);
	if (!v) {
		fprintf(stderr, "%s: valeur=%s : la variable %s n'existe pas dans ce dialogue\n", programme, brute, nom);
		return NULL;
	}
	static char ligne[256];
	snprintf(ligne, sizeof ligne, "%s", v);
	ligne[strcspn(ligne, "\r\n")] = '\0';
	return ligne;
}
