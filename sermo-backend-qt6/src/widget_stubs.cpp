/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_stubs.cpp — (vide) Qt6
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Historiquement ce fichier fournissait des stubs (QWidget vide) pour les
 * tags non encore portés sur Qt6. Ils sont DÉSORMAIS TOUS implémentés
 * nativement dans leurs propres fichiers :
 *   <eventbox>    → widget_eventbox.cpp
 *   <filechooser> → widget_filechooser.cpp
 *   <linkbutton>  → widget_linkbutton.cpp
 *   <image>       → widget_image.cpp
 *   <pulse>       → widget_pulse.cpp
 *   <menu>        → widget_menuitem.cpp (QMenu)
 *
 * Plus aucun widget n'est stubbé sur ce port ; ce fichier ne contient donc
 * plus de définition. Il est conservé (référencé par CMakeLists) pour ne pas
 * perturber l'historique de build.
 */

/* Évite toute unité de traduction « vide » selon les compilateurs. */
typedef int qt6sermo_widget_stubs_no_longer_used;
