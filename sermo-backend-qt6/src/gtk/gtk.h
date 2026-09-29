/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* gtk/gtk.h — stub du port Qt6 (qt6sermo)
 *
 * Le port Qt6 ne dépend PAS de GTK : l'API GTK/GLib utilisée par le cœur
 * sermo est fournie par sa couche de compatibilité. Le lexer et le parser
 * de l'ÉTALON (cœur partagé) contiennent
 * « #include <gtk/gtk.h> » ; src/ étant sur le chemin d'inclusion, ce stub
 * est résolu en premier et neutralise la dépendance aux en-têtes GTK réels.
 *
 * haplo-dialog / qt6sermo — GPL-2.0-or-later
 */
#ifndef SERMO_QT6_GTK_STUB_H
#define SERMO_QT6_GTK_STUB_H
#ifndef __GTK_H__
#define __GTK_H__
#endif
#ifndef __GTKX_H__
#define __GTKX_H__
#endif
#endif /* SERMO_QT6_GTK_STUB_H */
