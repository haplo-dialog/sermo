/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mavlink-commun.c — adresse et ouverture du lien MAVLink. Contrat :
 * mavlink-commun.h.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "mavlink-commun.h"

long
mavlink_maintenant_ms(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

speed_t
mavlink_debit(long v)
{
	switch (v) {
	case 9600: return B9600;
	case 19200: return B19200;
	case 38400: return B38400;
	case 57600: return B57600;
	case 115200: return B115200;
	case 230400: return B230400;
	case 460800: return B460800;
	case 921600: return B921600;
	default: return 0;
	}
}

/* hôte:port, hôte pouvant être vide (UDP : 127.0.0.1) ou [IPv6]. */
static int
hote_port(const char *texte, lien_t *l, int hote_vide_permis)
{
	const char *deux;
	size_t nh;
	if (*texte == '[') {
		const char *f = strchr(texte, ']');
		if (!f || f[1] != ':')
			return -1;
		nh = (size_t)(f - texte - 1);
		if (nh == 0 || nh >= sizeof l->hote)
			return -1;
		memcpy(l->hote, texte + 1, nh);
		deux = f + 1;
	} else {
		deux = strrchr(texte, ':');
		if (!deux)
			return -1;
		nh = (size_t)(deux - texte);
		if (nh >= sizeof l->hote || strchr(texte, '/'))
			return -1;
		memcpy(l->hote, texte, nh);
	}
	l->hote[nh] = '\0';
	if (nh == 0) {
		if (!hote_vide_permis)
			return -1;
		snprintf(l->hote, sizeof l->hote, "127.0.0.1");
	}
	char *fin;
	long p = strtol(deux + 1, &fin, 10);
	if (deux[1] == '\0' || *fin || p < 1 || p > 65535)
		return -1;
	snprintf(l->port, sizeof l->port, "%ld", p);
	return 0;
}

int
lien_analyser(const char *programme, const char *base, lien_t *l)
{
	l->base = base;
	if (strncmp(base, "udp://", 6) == 0) {
		l->transport = LIEN_UDP;
		if (hote_port(base + 6, l, 1) != 0) {
			fprintf(stderr, "%s: adresse UDP attendue : udp://[hôte]:port (exemple : udp://:14550)\n", programme);
			return -1;
		}
	} else if (strncmp(base, "tcp://", 6) == 0) {
		l->transport = LIEN_TCP;
		if (hote_port(base + 6, l, 0) != 0) {
			fprintf(stderr, "%s: adresse TCP attendue : tcp://hôte:port (exemple : tcp://127.0.0.1:5760)\n", programme);
			return -1;
		}
	} else if (strncmp(base, "serie://", 8) == 0) {
		l->transport = LIEN_SERIE;
		if (base[8] != '/' || strlen(base + 8) >= sizeof l->chemin) {
			fprintf(stderr, "%s: adresse série attendue : serie:///dev/ttyUSB0\n", programme);
			return -1;
		}
		snprintf(l->chemin, sizeof l->chemin, "%s", base + 8);
	} else {
		fprintf(stderr, "%s: l'adresse commence par udp://, tcp:// ou serie://\n", programme);
		return -1;
	}
	return 0;
}

int
lien_ouvrir(const char *programme, const lien_t *l, long fin, int ecriture)
{
	if (l->transport == LIEN_SERIE) {
		int fd = open(l->chemin, (ecriture ? O_RDWR : O_RDONLY) | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
		if (fd < 0) {
			fprintf(stderr, "%s: %s illisible (%s)\n", programme, l->chemin, strerror(errno));
			return -1;
		}
		struct termios t;
		if (tcgetattr(fd, &t) == 0) {
			cfmakeraw(&t);
			cfsetispeed(&t, mavlink_debit(l->vitesse));
			cfsetospeed(&t, mavlink_debit(l->vitesse));
			t.c_cflag |= CLOCAL | CREAD;
			tcsetattr(fd, TCSANOW, &t);
		}
		return fd;
	}
	struct addrinfo indice, *res = NULL;
	memset(&indice, 0, sizeof indice);
	indice.ai_family = AF_UNSPEC;
	indice.ai_socktype = l->transport == LIEN_UDP ? SOCK_DGRAM : SOCK_STREAM;
	indice.ai_flags = l->transport == LIEN_UDP ? AI_PASSIVE : 0;
	int e = getaddrinfo(l->hote, l->port, &indice, &res);
	if (e != 0) {
		fprintf(stderr, "%s: %s : adresse introuvable (%s)\n", programme, l->base, gai_strerror(e));
		return -1;
	}
	int fd = -1, erreur = 0;
	for (struct addrinfo *a = res; a; a = a->ai_next) {
		fd = socket(a->ai_family, a->ai_socktype | SOCK_CLOEXEC | SOCK_NONBLOCK, a->ai_protocol);
		if (fd < 0)
			continue;
		if (l->transport == LIEN_UDP) {
			int un = 1;
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &un, sizeof un);
			if (bind(fd, a->ai_addr, a->ai_addrlen) == 0)
				break;
		} else {
			/* Connexion non bloquante, bornée par le délai, adresse par adresse. */
			if (connect(fd, a->ai_addr, a->ai_addrlen) == 0)
				break;
			if (errno == EINPROGRESS) {
				struct pollfd p = { fd, POLLOUT, 0 };
				long reste = fin - mavlink_maintenant_ms();
				int so = 0;
				socklen_t sl = sizeof so;
				if (reste > 0 && poll(&p, 1, (int)reste) == 1
				    && getsockopt(fd, SOL_SOCKET, SO_ERROR, &so, &sl) == 0 && so == 0)
					break;
				errno = so ? so : ETIMEDOUT;
			}
		}
		erreur = errno;
		close(fd);
		fd = -1;
	}
	freeaddrinfo(res);
	if (fd < 0)
		fprintf(stderr, "%s: %s : %s impossible (%s)\n", programme, l->base,
		        l->transport == LIEN_UDP ? "écoute" : "connexion", strerror(erreur ? erreur : EHOSTUNREACH));
	return fd;
}
