/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* sermo_shm_repli.c — shm_open/shm_unlink par memfd_create, pour un système
 * sans mémoire partagée POSIX (bionic d'Android). Compilé seulement quand
 * CMake ne trouve pas shm_open.
 *
 * Le seul appelant est le pilote Wayland de LVGL : un tampon anonyme, ouvert
 * en O_RDWR | O_CREAT | O_EXCL puis aussitôt défait par shm_unlink. memfd en
 * donne l'équivalent exact (un fichier en mémoire sans nom dans le système de
 * fichiers) ; shm_unlink n'a donc rien à défaire. Tout autre usage (ouvrir un
 * segment nommé existant) est refusé plutôt que simulé à faux.
 *
 * memfd_create est appelé par syscall() : la fonction de bionic n'existe
 * qu'à partir d'Android 11 (API 30), l'appel système depuis Linux 3.17. */
#define _GNU_SOURCE
#include "sermo_shm_repli.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif

int shm_open(const char *name, int oflag, mode_t mode)
{
	(void)mode;
	if (name == NULL || (oflag & O_ACCMODE) != O_RDWR || !(oflag & O_CREAT)) {
		errno = ENOSYS;
		return -1;
	}
	if (name[0] == '/')
		name++;
	return (int)syscall(SYS_memfd_create, name, MFD_CLOEXEC);
}

int shm_unlink(const char *name)
{
	(void)name;
	return 0;
}
