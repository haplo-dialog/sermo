/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* gtk/gtk.h — stub du port FLTK (fltk1dialog)
 *
 * Le port FLTK ne dépend PAS de GTK : toute l'API GTK/GLib utilisée par le
 * cœur gtkdialog est fournie sous forme de types et de macros par
 * fltk-compat.h, qui est force-inclus via « -include ../src/fltk-compat.h »
 * (voir CMakeLists.txt). Les fichiers source historiques contiennent encore
 * « #include <gtk/gtk.h> » ; comme la compilation s'effectue dans src/ avec
 * « -I. », cet en-tête de remplacement est résolu en premier et neutralise
 * la dépendance aux en-têtes GTK réels (absents sur les systèmes sans GTK).
 *
 * haplo-dialog / fltk1dialog 1.0.0 — GPL-2.0-or-later
 */
#ifndef SERMO_FLTK_GTK_STUB_H
#define SERMO_FLTK_GTK_STUB_H
/* __GTK_H__ / __GTKX_H__ : si un vrai gtk/gtk.h est trouvé ailleurs, son
 * corps sera neutralisé par ces gardes déjà définies. */
#ifndef __GTK_H__
#define __GTK_H__
#endif
#ifndef __GTKX_H__
#define __GTKX_H__
#endif
/* Rien d'autre : fltk-compat.h fournit déjà tous les types et macros. */
#endif /* SERMO_FLTK_GTK_STUB_H */
