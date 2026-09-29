# SPDX-License-Identifier: GPL-2.0-or-later
# sermo-backend-gtk4 — backend de rendu GTK4, lié sur libsermocore VARIANTE GTK4
# (voie (b)) : le cœur utilise les fichiers réécrits de gtk4sermo ; le
# backend ne garde que ses widgets + la glue d'évènements GTK4.
cmake_minimum_required(VERSION 3.16)
# Version : une seule source, le fichier VERSION à la racine du dépôt. Rien
# n'est à resynchroniser ici ; tests/garde_version.sh vérifie que tout le dépôt
# s'y accorde.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)

project(gtk4sermo VERSION ${SERMO_VERSION} LANGUAGES C)

# FORTIFY_SOURCE n'agit qu'avec l'optimisation : sans type de build, CMake ne
# passe aucun -O et la protection promise par SECURITY.md reste lettre morte.
# Même règle que qt6, sdl3 et ncurses. Vérifié par tests/garde_fortify_coeur.sh
# (cœur) et tests/garde_durcissement.sh (binaires).
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4 glib-2.0 gthread-2.0)
pkg_check_modules(SERMOCORE REQUIRED sermocore)   # variante GTK4 (PKG_CONFIG_PATH=_install_gtk4)

# <terminal> : le cœur a tranché (variable « vte » de sermocore.pc) et le backend
# suit. ⚠️ Jusqu'à la 2.7.0, HAVE_VTE était forcé à 0 ici et dans le cœur : le
# terminal de la 1.x avait disparu sans un message.
pkg_get_variable(SERMOCORE_VTE sermocore vte)
if(SERMOCORE_VTE STREQUAL "1")
  pkg_check_modules(VTE REQUIRED vte-2.91-gtk4)
  set(HAVE_VTE 1)
  message(STATUS "gtk4sermo : <terminal> par vte-2.91-gtk4 ${VTE_VERSION}")
else()
  set(HAVE_VTE 0)
  message(WARNING "gtk4sermo : cœur construit sans VTE — <terminal> n'affichera qu'un avertissement")
endif()

file(GLOB WIDGET_SRC "src/widget*.c")             # widget_*.c ET widgets.c
set(BACKEND_SRC
  ${WIDGET_SRC}
  src/gtk4-events.c      # translation d'évènements GTK4
  src/printing.c
  src/glade_support.c    # --glade-xml : fichiers GtkBuilder (SERMO_GLADE du cœur)
)
# Retirés (désormais DANS le cœur variante GTK4) : gtk4-compat.c, tag_set_property.c,
# sermo_backend_gtk4sermo.c (main/init/globales), gtk4_neutral_abi.c.

add_executable(gtk4sermo ${BACKEND_SRC})
target_include_directories(gtk4sermo PRIVATE
  src/ ${SERMOCORE_INCLUDE_DIRS} ${GTK4_INCLUDE_DIRS} ${VTE_INCLUDE_DIRS})
target_compile_options(gtk4sermo PRIVATE
  -std=gnu11 ${GTK4_CFLAGS_OTHER}
  -Wno-deprecated-declarations   # appels GTK 4 dépréciés : chantier suivi à part
  -include ${CMAKE_CURRENT_SOURCE_DIR}/src/gtk4-compat.h
  -DHAVE_VTE=${HAVE_VTE}
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection -fcf-protection=full)
# Durcissement au lien : PIE/RELRO/BIND_NOW/NX + CET (IBT/SHSTK forcés, cf. qt6).
# Vérifié par tests/garde_durcissement.sh.
target_link_options(gtk4sermo PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
  -Wl,-z,ibt -Wl,-z,shstk
  -fcf-protection=full -fstack-protector-strong -fstack-clash-protection)
target_link_libraries(gtk4sermo ${SERMOCORE_LINK_LIBRARIES} ${GTK4_LIBRARIES} ${VTE_LIBRARIES})
target_link_directories(gtk4sermo PRIVATE ${GTK4_LIBRARY_DIRS} ${VTE_LIBRARY_DIRS})


# ─── Installation ─────────────────────────────────────────────────────────────
# Ces règles manquaient : seuls qt6 et le cœur en avaient. Qui construit depuis
# les sources n'avait aucun moyen d'installer le résultat — binaire, page de
# manuel et icônes étaient à recopier à la main, et COMPILE.md ne pouvait que
# désigner le binaire dans _build/. debian/rules pose ces mêmes fichiers de son
# côté : les deux chemins livrent désormais la même chose.
set(PACKAGE_VERSION "${PROJECT_VERSION}")
install(TARGETS gtk4sermo DESTINATION bin)

# Page de manuel propre au port, substituée depuis son modèle.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/gtk4sermo.1.in"
  "${CMAKE_CURRENT_BINARY_DIR}/gtk4sermo.1"
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gtk4sermo.1 DESTINATION share/man/man1)

# Page de section 5 : le format XML.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/gtk4sermo-xml.5.in"
  "${CMAKE_CURRENT_BINARY_DIR}/gtk4sermo-xml.5"
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gtk4sermo-xml.5 DESTINATION share/man/man5)

# Icône d'application, aux huit tailles du thème hicolor. OPTIONAL : une taille
# absente ne doit pas faire échouer l'installation.
foreach(taille 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256)
  install(FILES data/icons/hicolor/${taille}/apps/gtk4sermo.png
    DESTINATION share/icons/hicolor/${taille}/apps
    OPTIONAL)
endforeach()


# ─── Le port se nomme lui-même ────────────────────────────────────────────────
# « --version » est imprimé par le cœur, compilé une fois : il ne connaissait
# que son propre nom, et les sept ports s'annonçaient « sermocore ».
# contract/sermo_port_id.c remplit les deux symboles faibles que le cœur
# déclare, avec le nom et la version que CMake connaît déjà. La POSITION des
# mots ne bouge pas : seul le premier change (des scripts lisent le 3e).
target_sources(gtk4sermo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract/sermo_port_id.c)
target_compile_definitions(gtk4sermo PRIVATE
  SERMO_PORT_NAME="${PROJECT_NAME}"
  SERMO_PORT_VERSION="${PROJECT_VERSION}")
