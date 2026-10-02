/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SERMOCORE_CONFIG_H
#define SERMOCORE_CONFIG_H
#define PACKAGE "sermocore"
#define PACKAGE_NAME "sermocore"
#define BUILD_DETAILS "sermo/libsermocore"

#define PACKAGE_STRING "sermocore"
/* La version affichée par --version vient de project(VERSION) du CMakeLists.txt
 * du cœur, imposée à la compilation. La valeur ci-dessous ne sert qu'aux
 * programmes qui incluent cet en-tête sans passer par ce build. */
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "2.7.5"
#endif
#ifndef VERSION
#define VERSION PACKAGE_VERSION
#endif
#define PACKAGE_BUGREPORT "devel@haplo-dialog.fr"
#endif
