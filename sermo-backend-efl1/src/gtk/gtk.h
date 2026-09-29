/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* gtk/gtk.h — stub du port EFL (efl1sermo)
 *
 * Le port EFL ne dépend PAS de GTK : l'API GTK/GLib utilisée par le cœur
 * sermo est fournie par efl-compat.h, force-inclus via
 * « -include ../src/efl-compat.h » (voir CMakeLists.txt). Le lexer et le
 * parser de l'ÉTALON (cœur partagé) contiennent
 * « #include <gtk/gtk.h> » ; la compilation s'effectuant avec « -I. »,
 * ce stub est résolu en premier et neutralise la dépendance aux en-têtes
 * GTK réels.
 *
 * haplo-dialog / efl1sermo 1.0.0 — GPL-2.0-or-later
 */
#ifndef SERMO_EFL_GTK_STUB_H
#define SERMO_EFL_GTK_STUB_H
#ifndef __GTK_H__
#define __GTK_H__
#endif
#ifndef __GTKX_H__
#define __GTKX_H__
#endif
/* Rien d'autre : efl-compat.h fournit déjà tous les types et macros. */
#endif /* SERMO_EFL_GTK_STUB_H */
