/* SPDX-License-Identifier: MIT */
/*
 * sermo_port_id.c — le port se nomme lui-même.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * POURQUOI CE FICHIER EXISTE
 *
 * « --version » est imprimé par le cœur (gtkdialog.c), compilé une fois en
 * bibliothèque statique. Il n'y connaissait que ses propres PACKAGE_NAME et
 * BUILD_DETAILS : les sept ports s'annonçaient donc « sermocore version 2.7.1
 * sermo/libsermocore », aucun sous son nom. En 1.x chaque port compilait son
 * gtkdialog.c et se nommait juste ; le cœur partagé a emporté cette justesse
 * sans que personne ne le remarque — ci/construire.sh contrôle « --version »
 * sur chaque binaire, mais ne lit que le numéro.
 *
 * Ce fichier est compilé DANS chaque backend, avec le nom et la version que
 * CMake lui passe. Il remplit les deux symboles faibles que le cœur déclare :
 * un port qui ne le compilerait pas se lie quand même, et retombe sur les
 * valeurs du cœur. Rien n'est rompu pour un backend tiers.
 */

#ifndef SERMO_PORT_NAME
#  error "SERMO_PORT_NAME manquant : CMake doit le passer (voir les CMakeLists des ports)."
#endif
#ifndef SERMO_PORT_VERSION
#  error "SERMO_PORT_VERSION manquant : CMake doit le passer (voir les CMakeLists des ports)."
#endif

const char *sermo_port_name    = SERMO_PORT_NAME;
const char *sermo_port_details = SERMO_PORT_NAME " " SERMO_PORT_VERSION " (haplo-dialog)";
