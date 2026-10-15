/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * commun.c — adresse tout-en-un et pause après échec. Contrat : commun.h.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "commun.h"

static int
hexa(int c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

/* Décode %XX sur place. Rend -1 si un « % » n'est pas suivi de deux chiffres
 * hexadécimaux, ou si le décodage produit un octet nul ou de contrôle. */
static int
decoder(char *s)
{
	char *w = s;
	for (const char *r = s; *r; r++) {
		if (*r == '%') {
			int h = hexa((unsigned char)r[1]), l = h < 0 ? -1 : hexa((unsigned char)r[2]);
			if (l < 0)
				return -1;
			int c = h * 16 + l;
			if (c < 0x20 || c == 0x7f)
				return -1;
			*w++ = (char)c;
			r += 2;
		} else {
			*w++ = *r;
		}
	}
	*w = '\0';
	return 0;
}

int
adresse_lire(const char *programme, const char *brut, adresse_t *a)
{
	memset(a, 0, sizeof *a);
	if (!brut || !*brut || strlen(brut) > 3 * 4096) {
		fprintf(stderr, "%s: adresse vide ou trop longue\n", programme);
		return -1;
	}
	a->tampon = strdup(brut);
	if (!a->tampon || decoder(a->tampon) != 0) {
		fprintf(stderr, "%s: adresse mal encodée\n", programme);
		adresse_liberer(a);
		return -1;
	}
	a->base = a->tampon;
	char *q = strchr(a->tampon, '?');
	if (!q)
		return 0;
	*q++ = '\0';
	while (q && *q) {
		char *suite = strchr(q, '&');
		if (suite)
			*suite++ = '\0';
		if (*q) {
			char *egal = strchr(q, '=');
			if (!egal || egal == q) {
				fprintf(stderr, "%s: paramètre sans « clé=valeur » : %s\n", programme, q);
				adresse_liberer(a);
				return -1;
			}
			if (a->n == PARAMETRES_MAX) {
				fprintf(stderr, "%s: plus de %d paramètres\n", programme, PARAMETRES_MAX);
				adresse_liberer(a);
				return -1;
			}
			*egal = '\0';
			if (decoder(egal + 1) != 0) {
				fprintf(stderr, "%s: valeur mal encodée pour « %s »\n", programme, q);
				adresse_liberer(a);
				return -1;
			}
			a->cle[a->n] = q;
			a->valeur[a->n] = egal + 1;
			a->n++;
		}
		q = suite;
	}
	return 0;
}

void
adresse_liberer(adresse_t *a)
{
	free(a->tampon);
	memset(a, 0, sizeof *a);
}

const char *
adresse_valeur(const adresse_t *a, const char *cle)
{
	const char *v = NULL;
	for (size_t i = 0; i < a->n; i++)
		if (strcmp(a->cle[i], cle) == 0)
			v = a->valeur[i];
	return v;
}

int
adresse_cles_connues(const char *programme, const adresse_t *a, const char *const *connues)
{
	for (size_t i = 0; i < a->n; i++) {
		const char *const *k = connues;
		while (*k && strcmp(*k, a->cle[i]) != 0)
			k++;
		if (!*k) {
			fprintf(stderr, "%s: paramètre inconnu : %s\n", programme, a->cle[i]);
			return -1;
		}
	}
	return 0;
}

int
adresse_entier(const char *programme, const adresse_t *a, const char *cle,
               long min, long max, long defaut, long *sortie)
{
	const char *v = adresse_valeur(a, cle);
	if (!v) {
		*sortie = defaut;
		return 0;
	}
	char *fin;
	errno = 0;
	long n = strtol(v, &fin, 10);
	if (errno || fin == v || *fin || n < min || n > max) {
		fprintf(stderr, "%s: %s doit être un entier de %ld à %ld (lu : %s)\n", programme, cle, min, max, v);
		return -1;
	}
	*sortie = n;
	return 0;
}

/* Fichier témoin : $XDG_RUNTIME_DIR/sermo-source/<protocole>-<empreinte>. Sans
 * XDG_RUNTIME_DIR (privé à l'utilisateur), pas de pause : on ne pose rien dans
 * un dossier partagé. */
static int chemin_fichier(const char *protocole, const char *genre, const char *cle,
                          char *chemin, size_t taille);

static int
chemin_temoin(const char *protocole, const char *base, char *chemin, size_t taille)
{
	return chemin_fichier(protocole, "", base, chemin, taille);
}

/* $XDG_RUNTIME_DIR/sermo-source/<protocole>-<genre><empreinte de @cle>. */
static int
chemin_fichier(const char *protocole, const char *genre, const char *cle, char *chemin, size_t taille)
{
	const char *base = cle;
	const char *dossier = getenv("XDG_RUNTIME_DIR");
	if (!dossier || dossier[0] != '/')
		return -1;
	/* FNV-1a 64 bits : un nom de fichier stable, sans caractère de la base. */
	uint64_t h = 1469598103934665603ULL;
	for (const unsigned char *p = (const unsigned char *)base; *p; p++) {
		h ^= *p;
		h *= 1099511628211ULL;
	}
	int n = snprintf(chemin, taille, "%s/sermo-source", dossier);
	if (n < 0 || (size_t)n >= taille)
		return -1;
	if (mkdir(chemin, 0700) != 0 && errno != EEXIST)
		return -1;
	n = snprintf(chemin, taille, "%s/sermo-source/%s-%s%016llx", dossier, protocole, genre, (unsigned long long)h);
	return (n < 0 || (size_t)n >= taille) ? -1 : 0;
}

int
pause_active(const char *protocole, const char *base)
{
	char chemin[4096];
	struct stat st;
	if (chemin_temoin(protocole, base, chemin, sizeof chemin) != 0 || stat(chemin, &st) != 0)
		return 0;
	time_t maintenant = time(NULL);
	return st.st_mtime <= maintenant && maintenant - st.st_mtime < PAUSE_SECONDES;
}

void
pause_poser(const char *protocole, const char *base)
{
	char chemin[4096];
	if (chemin_temoin(protocole, base, chemin, sizeof chemin) != 0)
		return;
	int fd = open(chemin, O_WRONLY | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (fd >= 0) {
		/* Date de l'échec = maintenant, même si le témoin existait déjà. */
		futimens(fd, NULL);
		close(fd);
	}
}

void
pause_lever(const char *protocole, const char *base)
{
	char chemin[4096];
	if (chemin_temoin(protocole, base, chemin, sizeof chemin) == 0)
		unlink(chemin);
}

/* Millisecondes écoulées depuis la dernière modification de @chemin ; -1 si
 * absent, ou daté du futur (horloge reculée : on ne s'y fie pas). */
static long long
age_ms(const char *chemin)
{
	struct stat st;
	struct timespec maintenant;
	if (stat(chemin, &st) != 0 || clock_gettime(CLOCK_REALTIME, &maintenant) != 0)
		return -1;
	long long ms = (long long)(maintenant.tv_sec - st.st_mtim.tv_sec) * 1000
	               + (maintenant.tv_nsec - st.st_mtim.tv_nsec) / 1000000;
	return ms < 0 ? -1 : ms;
}

int
cache_servir(const char *protocole, const char *adresse, long fraicheur_ms)
{
	char chemin[4096];
	if (fraicheur_ms <= 0 || chemin_fichier(protocole, "c", adresse, chemin, sizeof chemin) != 0)
		return 0;
	long long age = age_ms(chemin);
	if (age < 0 || age >= fraicheur_ms)
		return 0;
	int fd = open(chemin, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	if (fd < 0)
		return 0;
	char tampon[65536];
	ssize_t n = read(fd, tampon, sizeof tampon);
	close(fd);
	/* Vide ou tronqué (plus de 64 Kio) : on relit l'équipement. */
	if (n <= 0 || (size_t)n == sizeof tampon)
		return 0;
	return fwrite(tampon, 1, (size_t)n, stdout) == (size_t)n && fflush(stdout) == 0;
}

void
cache_garder(const char *protocole, const char *adresse, const char *donnees, size_t n)
{
	char chemin[4096], provisoire[4200];
	if (n == 0 || n >= 65536 || chemin_fichier(protocole, "c", adresse, chemin, sizeof chemin) != 0)
		return;
	int ecrit = snprintf(provisoire, sizeof provisoire, "%s.XXXXXX", chemin);
	if (ecrit < 0 || (size_t)ecrit >= sizeof provisoire)
		return;
	int fd = mkstemp(provisoire);
	if (fd < 0)
		return;
	int ok = write(fd, donnees, n) == (ssize_t)n;
	ok = close(fd) == 0 && ok;
	if (!ok || rename(provisoire, chemin) != 0)
		unlink(provisoire);
}

int
verrou_prendre(const char *protocole, const char *base)
{
	char chemin[4096];
	if (chemin_fichier(protocole, "v", base, chemin, sizeof chemin) != 0)
		return -1;
	int fd = open(chemin, O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (fd < 0)
		return -1;
	while (flock(fd, LOCK_EX) != 0) {
		if (errno != EINTR) {
			close(fd);
			return -1;
		}
	}
	return fd;
}

void
verrou_rendre(int fd)
{
	if (fd >= 0)
		close(fd);   /* ferme et relâche le verrou */
}

/* Âge de la réponse gardée pour @adresse, en ms ; -1 si aucune. */
static long long
cache_age(const char *protocole, const char *adresse)
{
	char chemin[4096];
	if (chemin_fichier(protocole, "c", adresse, chemin, sizeof chemin) != 0)
		return -1;
	return age_ms(chemin);
}

/* Témoin « une lecture a déjà été tentée pour cette adresse ». */
static int
tentative_connue(const char *protocole, const char *adresse)
{
	char chemin[4096];
	struct stat st;
	return chemin_fichier(protocole, "a", adresse, chemin, sizeof chemin) == 0 && stat(chemin, &st) == 0;
}

static void
tentative_noter(const char *protocole, const char *adresse)
{
	char chemin[4096];
	if (chemin_fichier(protocole, "a", adresse, chemin, sizeof chemin) != 0)
		return;
	int fd = open(chemin, O_WRONLY | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (fd >= 0) {
		futimens(fd, NULL);
		close(fd);
	}
}

/* Lecture directe, sous le verrou de l'équipement. @garder : la réponse est
 * gardée pour les autres widgets. @ecrire : la réponse part sur la sortie
 * standard (non en fond). Rend 0 si une valeur a été obtenue. */
static int
lecture_directe(const char *programme, const char *protocole, const char *adresse, const char *base,
                long cache_ms, int garder, int ecrire, lecteur_t lire, void *donnees)
{
	int verrou = verrou_prendre(protocole, base);
	int code = 1;
	char *texte = NULL;
	size_t taille = 0;

	/* Un autre widget a peut-être lu pendant qu'on attendait le verrou. */
	if (garder && ecrire && cache_servir(protocole, adresse, cache_ms)) {
		code = 0;
		goto fin;
	}
	if (pause_active(protocole, base)) {
		if (ecrire)
			fprintf(stderr, "%s: %s n'a pas répondu il y a moins de %d s : pas de nouvel essai\n",
			        programme, base, PAUSE_SECONDES);
		goto fin;
	}
	tentative_noter(protocole, adresse);
	FILE *sortie = open_memstream(&texte, &taille);
	if (!sortie) {
		fprintf(stderr, "%s: mémoire épuisée\n", programme);
		goto fin;
	}
	lecture_t r = lire(donnees, sortie);
	if (fclose(sortie) != 0)
		r = LECTURE_REFUS;
	if (r == LECTURE_PANNE)
		pause_poser(protocole, base);
	if (r != LECTURE_OK)
		goto fin;
	pause_lever(protocole, base);
	if (garder)
		cache_garder(protocole, adresse, texte, taille);
	if (!ecrire || (fwrite(texte, 1, taille, stdout) == taille && fflush(stdout) == 0))
		code = 0;
fin:
	free(texte);
	verrou_rendre(verrou);
	return code;
}

/* Relecture en fond : un processus détaché, dont la sortie standard est
 * fermée AVANT tout travail (sermo lit le tube jusqu'à sa fin : un enfant qui
 * le garderait ouvert le ferait attendre). Une seule relecture par adresse à
 * la fois. Le parent n'attend rien. */
static void
relecture_en_fond(const char *programme, const char *protocole, const char *adresse, const char *base,
                  long cache_ms, lecteur_t lire, void *donnees)
{
	fflush(stdout);
	fflush(stderr);
	pid_t pid = fork();
	if (pid != 0)
		return;   /* parent (ou échec du fork : on renonce à relire) */
	int nul = open("/dev/null", O_RDWR | O_CLOEXEC);
	if (nul >= 0) {
		dup2(nul, STDIN_FILENO);
		dup2(nul, STDOUT_FILENO);
		dup2(nul, STDERR_FILENO);
		if (nul > STDERR_FILENO)
			close(nul);
	} else {
		close(STDOUT_FILENO);
	}
	setsid();
	char chemin[4096];
	int unique = -1;
	if (chemin_fichier(protocole, "r", adresse, chemin, sizeof chemin) == 0)
		unique = open(chemin, O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (unique < 0 || flock(unique, LOCK_EX | LOCK_NB) != 0)
		_exit(0);   /* une relecture de cette adresse est déjà en cours */
	long long age = cache_age(protocole, adresse);
	if (age >= 0 && age < cache_ms)
		_exit(0);   /* déjà rafraîchie par un autre */
	lecture_directe(programme, protocole, adresse, base, cache_ms, 1, 0, lire, donnees);
	_exit(0);
}

int
source_lire(const char *programme, const char *protocole, const char *adresse,
            const char *base, long cache_ms, long perime_ms, lecteur_t lire, void *donnees)
{
	if (cache_ms <= 0)
		return lecture_directe(programme, protocole, adresse, base, 0, 0, 1, lire, donnees);
	if (perime_ms < cache_ms)
		perime_ms = cache_ms;
	if (cache_servir(protocole, adresse, cache_ms))
		return 0;
	long long age = cache_age(protocole, adresse);
	if (age >= 0 && age < perime_ms) {
		/* Valeur périmée mais récente : rendue tout de suite, relue en fond. */
		relecture_en_fond(programme, protocole, adresse, base, cache_ms, lire, donnees);
		return cache_servir(protocole, adresse, perime_ms) ? 0 : 1;
	}
	if (tentative_connue(protocole, adresse)) {
		relecture_en_fond(programme, protocole, adresse, base, cache_ms, lire, donnees);
		fprintf(stderr, "%s: pas de valeur récente pour %s (relecture en cours)\n", programme, base);
		return 1;
	}
	return lecture_directe(programme, protocole, adresse, base, cache_ms, 1, 1, lire, donnees);
}
