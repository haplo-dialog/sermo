/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* sermo_shm_repli.h — déclarations de shm_open/shm_unlink pour un système qui
 * ne les a pas (bionic d'Android). Inclus de force dans LVGL embarqué, et
 * seulement là, quand CMake ne trouve pas shm_open : le pilote Wayland de
 * LVGL les appelle (tampons wl_shm) et le C99 refuse un appel non déclaré. */
#ifndef SERMO_SHM_REPLI_H
#define SERMO_SHM_REPLI_H

#include <sys/types.h>

int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);

#endif /* SERMO_SHM_REPLI_H */
