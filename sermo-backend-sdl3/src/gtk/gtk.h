/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* gtk/gtk.h — stub du port SDL3 (sdl3sermo)
 *
 * Le port SDL3 ne dépend PAS de GTK : l'API GTK/GLib utilisée par le cœur
 * sermo est fournie par sdl3-compat.h (force-inclus, voir CMakeLists.txt).
 * Le lexer et le parser de l'ÉTALON (cœur partagé) contiennent
 * « #include <gtk/gtk.h> » ; src/ étant sur le chemin d'inclusion, ce stub
 * est résolu en premier et neutralise la dépendance aux en-têtes GTK réels.
 *
 * haplo-dialog / sdl3sermo 1.0.0 — GPL-2.0-or-later
 */
#ifndef SERMO_SDL3_GTK_STUB_H
#define SERMO_SDL3_GTK_STUB_H
#ifndef __GTK_H__
#define __GTK_H__
#endif
#ifndef __GTKX_H__
#define __GTKX_H__
#endif
/* Rien d'autre : sdl3-compat.h fournit déjà tous les types et macros. */
#endif /* SERMO_SDL3_GTK_STUB_H */
