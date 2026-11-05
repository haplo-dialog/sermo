/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo-media-images — le lecteur de <action media> quand l'image va DANS
 * la zone du dialogue (dans=zone ; Wayland, où mpv --wid n'existe pas).
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Lancé par sermo-cible-media À LA PLACE de mpv, dans la même cage bwrap et
 * sous le même gardien : il prend les mêmes options (« --nom=valeur »,
 * « --no-nom », « -- » puis la source), plus --images=FICHIER. libmpv y
 * décode et joue le son comme mpv ; l'image est rendue en LOGICIEL (API de
 * rendu « sw ») dans FICHIER (voir sermo_images.h), que la zone du dialogue
 * lit. Aucun accès à l'écran : la cage n'en monte aucun dans ce mode.
 * Le socket de commandes (--input-ipc-server) est celui de mpv : pause,
 * aller, volume… marchent sans changement.
 *
 * Codes de sortie : 0 fin normale · 2 échec de lecture · 1 usage ou erreur.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <poll.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mpv/client.h>
#include <mpv/render.h>

#include "sermo_images.h"

#define PROGRAMME "sermo-media-images"

static int reveil[2] = { -1, -1 };              /* le rendu a une image prête */

static void
mise_a_jour(void *x)
{
	(void)x;
	char c = 1;
	if (write(reveil[1], &c, 1) < 0) { /* tube plein : un réveil est déjà en attente */ }
}

/* Le fichier d'images : créé à côté puis renommé (le lecteur voit l'ancien
 * ou le nouveau, jamais un fichier à moitié fait). */
static unsigned char *carte;
static size_t carte_taille;

static int
fichier_creer(const char *chemin, unsigned l, unsigned h)
{
	char tmp[4096];
	if (snprintf(tmp, sizeof tmp, "%s.n", chemin) >= (int)sizeof tmp)
		return -1;
	size_t taille = SERMO_IMAGES_DONNEES + 2u * (size_t)h * l * 4u;
	unlink(tmp);
	int fd = open(tmp, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd < 0)
		return -1;
	if (ftruncate(fd, (off_t)taille) != 0) {
		close(fd);
		unlink(tmp);
		return -1;
	}
	unsigned char *c = mmap(NULL, taille, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (c == MAP_FAILED) {
		unlink(tmp);
		return -1;
	}
	struct sermo_images_entete *e = (struct sermo_images_entete *)c;
	memcpy(e->magie, SERMO_IMAGES_MAGIE, 8);
	e->version = SERMO_IMAGES_VERSION;
	e->largeur = l;
	e->hauteur = h;
	e->pas = l * 4u;
	e->courant = 0;
	e->sequence = 0;
	if (rename(tmp, chemin) != 0) {
		munmap(c, taille);
		unlink(tmp);
		return -1;
	}
	if (carte)
		munmap(carte, carte_taille);
	carte = c;
	carte_taille = taille;
	return 0;
}

/* La taille de rendu : celle de la vidéo, réduite pour tenir dans le maximum. */
static void
taille_rendu(int64_t vl, int64_t vh, unsigned *l, unsigned *h)
{
	double r = 1.0;
	if (vl > SERMO_IMAGES_LARGEUR_MAX)
		r = (double)SERMO_IMAGES_LARGEUR_MAX / (double)vl;
	if (vh * r > SERMO_IMAGES_HAUTEUR_MAX)
		r = (double)SERMO_IMAGES_HAUTEUR_MAX / (double)vh;
	*l = (unsigned)(vl * r);
	*h = (unsigned)(vh * r);
	if (*l < 1) *l = 1;
	if (*h < 1) *h = 1;
}

int
main(int argc, char **argv)
{
	const char *images = NULL, *source = NULL;
	setlocale(LC_NUMERIC, "C");                    /* exigé par libmpv */
	mpv_handle *mpv = mpv_create();
	if (!mpv) {
		fputs(PROGRAMME ": libmpv indisponible\n", stderr);
		return 1;
	}
	/* Les options de mpv, telles que sermo-cible-media les écrit. */
	for (int i = 1; i < argc; i++) {
		const char *a = argv[i];
		if (strcmp(a, "--") == 0) {
			if (i + 1 < argc)
				source = argv[i + 1];
			break;
		}
		if (strncmp(a, "--", 2) != 0) {
			fprintf(stderr, PROGRAMME ": argument inattendu « %s »\n", a);
			return 1;
		}
		char nom[128];
		const char *eg = strchr(a + 2, '=');
		size_t n = eg ? (size_t)(eg - a - 2) : strlen(a + 2);
		if (n == 0 || n >= sizeof nom) {
			fprintf(stderr, PROGRAMME ": option « %s » refusée\n", a);
			return 1;
		}
		memcpy(nom, a + 2, n);
		nom[n] = '\0';
		if (strcmp(nom, "images") == 0) {
			images = eg ? eg + 1 : NULL;
			continue;
		}
		const char *val = eg ? eg + 1 : "yes";
		if (!eg && strncmp(nom, "no-", 3) == 0) {
			memmove(nom, nom + 3, strlen(nom + 3) + 1);
			val = "no";
		}
		/* libmpv : la source n'est chargée qu'après l'initialisation ; avec
		 * idle=no, mpv voit une liste vide et s'arrête aussitôt (mesuré).
		 * « once » : attendre la première source, s'arrêter à sa fin. */
		if (strcmp(nom, "idle") == 0 && strcmp(val, "no") == 0)
			val = "once";
		if (mpv_set_option_string(mpv, nom, val) < 0) {
			fprintf(stderr, PROGRAMME ": option mpv « %s » refusée\n", a);
			return 1;
		}
	}
	if (!images || images[0] != '/') {
		fputs(PROGRAMME ": --images=FICHIER (chemin absolu) manquant\n", stderr);
		return 1;
	}
	mpv_set_option_string(mpv, "vo", "libmpv");
	if (mpv_initialize(mpv) < 0) {
		fputs(PROGRAMME ": libmpv refuse de démarrer\n", stderr);
		return 1;
	}
	mpv_render_param init[] = {
		{ MPV_RENDER_PARAM_API_TYPE, (void *)MPV_RENDER_API_TYPE_SW },
		{ MPV_RENDER_PARAM_INVALID, NULL },
	};
	mpv_render_context *rc = NULL;
	if (pipe2(reveil, O_CLOEXEC | O_NONBLOCK) != 0 || mpv_render_context_create(&rc, mpv, init) < 0) {
		fputs(PROGRAMME ": rendu logiciel indisponible\n", stderr);
		mpv_terminate_destroy(mpv);
		return 1;
	}
	mpv_render_context_set_update_callback(rc, mise_a_jour, NULL);
	if (source) {
		const char *cmd[] = { "loadfile", source, NULL };
		mpv_command(mpv, cmd);
	}

	int code = 0, fin = 0;
	unsigned l = 0, h = 0;
	struct pollfd p[2] = { { .fd = reveil[0], .events = POLLIN }, { .fd = mpv_get_wakeup_pipe(mpv), .events = POLLIN } };
	while (!fin) {
		if (poll(p, 2, 1000) < 0 && errno != EINTR)
			break;
		if (p[0].revents & POLLIN) {
			char t[64];
			while (read(reveil[0], t, sizeof t) > 0) { }
		}
		if (p[1].revents & POLLIN) {
			char t[64];
			while (read(p[1].fd, t, sizeof t) > 0) { }
		}
		for (;;) {
			mpv_event *ev = mpv_wait_event(mpv, 0);
			if (ev->event_id == MPV_EVENT_NONE)
				break;
			if (ev->event_id == MPV_EVENT_SHUTDOWN) {
				fin = 1;
				break;
			}
			if (ev->event_id == MPV_EVENT_END_FILE) {
				mpv_event_end_file *f = ev->data;
				if (f && f->reason == MPV_END_FILE_REASON_ERROR)
					code = 2;
			}
			if (ev->event_id == MPV_EVENT_VIDEO_RECONFIG) {
				int64_t vl = 0, vh = 0;
				if (mpv_get_property(mpv, "dwidth", MPV_FORMAT_INT64, &vl) >= 0
				    && mpv_get_property(mpv, "dheight", MPV_FORMAT_INT64, &vh) >= 0 && vl > 0 && vh > 0) {
					unsigned nl, nh;
					taille_rendu(vl, vh, &nl, &nh);
					if ((nl != l || nh != h) && fichier_creer(images, nl, nh) == 0) {
						l = nl;
						h = nh;
					}
				}
			}
		}
		if (fin)
			break;
		if (!(mpv_render_context_update(rc) & MPV_RENDER_UPDATE_FRAME) || !carte)
			continue;
		struct sermo_images_entete *e = (struct sermo_images_entete *)carte;
		unsigned suivant = e->courant ^ 1u;
		int taille[2] = { (int)l, (int)h };
		size_t pas = (size_t)l * 4u;
		mpv_render_param r[] = {
			{ MPV_RENDER_PARAM_SW_SIZE, taille },
			{ MPV_RENDER_PARAM_SW_FORMAT, (void *)"bgr0" },
			{ MPV_RENDER_PARAM_SW_STRIDE, &pas },
			{ MPV_RENDER_PARAM_SW_POINTER, carte + SERMO_IMAGES_DONNEES + suivant * (size_t)h * pas },
			{ MPV_RENDER_PARAM_INVALID, NULL },
		};
		if (mpv_render_context_render(rc, r) < 0)
			continue;
		/* Publier : le tampon, puis le numéro de séquence (ordre garanti). */
		atomic_store_explicit((_Atomic uint32_t *)&e->courant, suivant, memory_order_release);
		atomic_store_explicit((_Atomic uint64_t *)&e->sequence, e->sequence + 1, memory_order_release);
	}
	mpv_render_context_free(rc);
	mpv_terminate_destroy(mpv);
	return code;
}
