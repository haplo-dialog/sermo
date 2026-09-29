/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_input.c — la limite de taille de ce que lit un <input>. Contrat et
 * raisons : include/sermo_input.h.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * La limite est posée sur le FLUX, par un flux « cookie » de la glibc
 * (fopencookie) : les lecteurs existants des sept ports — fgets, getline,
 * fread, fgetc — lisent comme avant, sans rien savoir de la limite.
 *
 * La lecture passe par read() sur le descripteur, pas par fread() sur le flux
 * d'origine : fread() attendrait d'avoir rempli tout le tampon demandé par la
 * glibc (plusieurs Kio) avant de rendre la main. read() rend ce qui est arrivé,
 * comme avant.
 *
 * C POSIX, sans GLib hormis g_warning() : la variante NEUTRE du cœur (qt6,
 * fltk1, efl1, sdl3, ncurses) ne lie pas la GLib, elle n'en imite qu'une
 * poignée de noms (include/sermocore-shim.h), dont g_warning().
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include "sermo_input.h"

typedef struct Plafond {
	FILE   *flux;       /* le flux réellement lu, fermé avec l'enveloppe */
	FILE   *enveloppe;  /* le flux plafonné rendu au lecteur */
	size_t  lu;         /* octets rendus jusqu'ici */
	size_t  max;        /* la limite */
	char   *source;     /* l'<input> nommé dans l'avertissement (NULL : « ? ») */
	int     prevenu;    /* l'avertissement est parti */
	int     detache;    /* sermo_input_sans_limite() a repris le flux réel */
	struct Plafond *suivant;
} Plafond;

/* Les flux plafonnés ouverts, pour sermo_input_sans_limite(). Les lectures des
 * ports peuvent tourner dans un fil à part : verrou. */
static Plafond         *ouverts;
static pthread_mutex_t  verrou = PTHREAD_MUTEX_INITIALIZER;

static pthread_once_t   une_fois = PTHREAD_ONCE_INIT;
static size_t           limite = SERMO_INPUT_MAX_DEFAUT;

static void lire_limite(void)
{
	const char         *s = getenv("SERMO_INPUT_MAX");
	const char         *c;
	char               *fin = NULL;
	unsigned long long  v;
	int                 lisible;

	if (s == NULL || *s == '\0')
		return;
	/* Des chiffres, rien d'autre : « -1 », « 16M », « 5 » entouré d'espaces ne
	 * passent pas (strtoull accepterait les trois). */
	for (c = s; *c >= '0' && *c <= '9'; c++)
		;
	errno = 0;
	v = strtoull(s, &fin, 10);
	lisible = (*c == '\0' && errno == 0 && fin != NULL && *fin == '\0');
#if ULLONG_MAX > SIZE_MAX
	lisible = lisible && v <= SIZE_MAX;
#endif
	if (lisible)
		limite = (size_t) v;
	else
		g_warning("SERMO_INPUT_MAX=« %s » illisible (un nombre d'octets est "
		          "attendu) : limite par défaut, %zu octets",
		          s, (size_t) SERMO_INPUT_MAX_DEFAUT);
}

size_t sermo_input_max(void)
{
	pthread_once(&une_fois, lire_limite);
	return limite;
}

static ssize_t lire_fd(int fd, char *tampon, size_t taille)
{
	ssize_t n;

	do {
		n = read(fd, tampon, taille);
	} while (n < 0 && errno == EINTR);
	return n;
}

static ssize_t lire(void *cookie, char *tampon, size_t taille)
{
	Plafond *p = cookie;
	int      fd = fileno(p->flux);
	ssize_t  n;

	if (p->lu >= p->max) {
		/* La limite est atteinte. Un octet de plus dit s'il y a troncature :
		 * une source qui fait exactement la limite n'est pas signalée. */
		if (!p->prevenu) {
			char octet;

			if (lire_fd(fd, &octet, 1) == 1) {
				p->prevenu = 1;
				g_warning("<input> « %s » : lecture arrêtée à %zu octets "
				          "(limite SERMO_INPUT_MAX)",
				          p->source != NULL ? p->source : "?", p->max);
			}
		}
		return 0;   /* fin de fichier pour le lecteur */
	}

	if (taille > p->max - p->lu)
		taille = p->max - p->lu;
	n = lire_fd(fd, tampon, taille);
	if (n > 0)
		p->lu += (size_t) n;
	return n;
}

static int fermer(void *cookie)
{
	Plafond  *p = cookie;
	Plafond **q;
	int       r;

	pthread_mutex_lock(&verrou);
	for (q = &ouverts; *q != NULL; q = &(*q)->suivant)
		if (*q == p) {
			*q = p->suivant;
			break;
		}
	pthread_mutex_unlock(&verrou);

	/* Détaché, le flux réel appartient désormais à l'appelant. */
	r = p->detache ? 0 : fclose(p->flux);
	free(p->source);
	free(p);
	return r;
}

/* Une commande peut être longue : l'avertissement n'en cite que le début,
 * coupé sur une frontière de caractère UTF-8, jamais au milieu d'un « é ». */
static char *nommer(const char *source)
{
	size_t n;
	char  *nom;

	if (source == NULL)
		return NULL;
	n = strlen(source);
	if (n <= 120)
		return strdup(source);
	n = 117;
	while (n > 0 && ((unsigned char) source[n] & 0xC0) == 0x80)
		n--;
	nom = malloc(n + sizeof "…");
	if (nom != NULL) {
		memcpy(nom, source, n);
		memcpy(nom + n, "…", sizeof "…");
	}
	return nom;
}

FILE *sermo_input_wrap(FILE *flux, const char *source)
{
	cookie_io_functions_t fonctions = { lire, NULL, NULL, fermer };
	Plafond *p;
	size_t   max;

	if (flux == NULL)
		return NULL;
	max = sermo_input_max();
	if (max == 0)
		return flux;   /* SERMO_INPUT_MAX=0 : sans limite, comme avant la 2.7.3 */

	p = calloc(1, sizeof *p);
	if (p == NULL)
		return flux;   /* mémoire épuisée : le flux sans limite plutôt que perdu */
	p->flux = flux;
	p->max = max;
	p->source = nommer(source);

	p->enveloppe = fopencookie(p, "r", fonctions);
	if (p->enveloppe == NULL) {
		free(p->source);
		free(p);
		return flux;
	}

	pthread_mutex_lock(&verrou);
	p->suivant = ouverts;
	ouverts = p;
	pthread_mutex_unlock(&verrou);
	return p->enveloppe;
}

FILE *sermo_fopen_input(const char *chemin)
{
	if (chemin == NULL)
		return NULL;
	return sermo_input_wrap(fopen(chemin, "r"), chemin);
}

FILE *sermo_input_sans_limite(FILE *flux)
{
	Plafond *p;
	FILE    *brut;

	if (flux == NULL)
		return NULL;
	pthread_mutex_lock(&verrou);
	for (p = ouverts; p != NULL && p->enveloppe != flux; p = p->suivant)
		;
	pthread_mutex_unlock(&verrou);
	if (p == NULL)
		return flux;   /* un flux ordinaire, ou SERMO_INPUT_MAX=0 */

	brut = p->flux;
	p->detache = 1;
	fclose(flux);      /* fermer() libère l'enveloppe et laisse le flux réel ouvert */
	return brut;
}
