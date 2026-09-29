# SPDX-License-Identifier: GPL-2.0-or-later
# sermo-backend-ncurses — backend de rendu TERMINAL (ncurses), lié sur
# libsermocore (variante NEUTRE). Le CŒUR (parse/automaton/variables/actions/
# gtkdialog.c+main/lexer/parser) vit dans libsermocore ; ce backend ne garde que
# ses widgets (modèle arbre-de-WidgetNode partagé avec le port sdl3), le bridge
# d'attributs (tag_set_property.c), le hook d'init + l'ABI neutre
# (sermo_backend_ncurses.c) et le renderer/boucle ncurses (render_ncurses.c).
cmake_minimum_required(VERSION 3.16)
# Version : une seule source, le fichier VERSION à la racine du dépôt. Rien
# n'est à resynchroniser ici ; tests/garde_version.sh vérifie que tout le dépôt
# s'y accorde.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)

project(ncursessermo VERSION ${SERMO_VERSION} LANGUAGES C)

# config.h est ENGENDRÉ ici, pas versionné : la copie écrite à la main portait
# la version en dur, avec la consigne de la « garder en accord » avec
# CMakeLists.txt. Elle suit désormais project(... VERSION ...), donc VERSION.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
  @ONLY)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ─── Cœur partagé via pkg-config (variante NEUTRE) ─────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(SERMOCORE REQUIRED sermocore)
pkg_check_modules(NCURSES   REQUIRED ncursesw)

# ─── Sources backend ───────────────────────────────────────────────────────
# widget*.c capture widget_*.c + widgets.c + widget_node.c + widget_stubs.c.
file(GLOB WIDGET_SRC "src/widget*.c")

# neutral_glist.c : fournit g_list_append EXTERNE au cœur (le shim ncurses-compat.h
# ne donne qu'un static inline, aucun symbole exporté). Compilé SANS le
# force-include du shim, sinon conflit de définition. Objet à part (cf. sdl3).
add_library(sermo_neutral_glist OBJECT src/neutral_glist.c)
target_compile_options(sermo_neutral_glist PRIVATE -fPIE)

add_executable(ncursessermo
  ${WIDGET_SRC}
  src/tag_set_property.c
  src/printing.c
  src/sermo_backend_ncurses.c
  src/render_ncurses.c
  $<TARGET_OBJECTS:sermo_neutral_glist>
)

target_include_directories(ncursessermo BEFORE PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}                  # config.h engendré
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract      # contrat sermo_be_* (MIT), rapatrie dans le depot
  ${CMAKE_CURRENT_SOURCE_DIR}/src        # en-têtes locaux (mêmes que le cœur neutre)
  ${SERMOCORE_INCLUDE_DIRS}
  ${NCURSES_INCLUDE_DIRS})

target_compile_options(ncursessermo PRIVATE
  -std=gnu11
  # ncurses-compat.h : shim GLib auto-suffisant (GList, g_*), pas de glib réelle
  -include ${CMAKE_CURRENT_SOURCE_DIR}/src/ncurses-compat.h
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection -fcf-protection=full
  ${NCURSES_CFLAGS_OTHER})

# Durcissement au lien : PIE/RELRO/BIND_NOW/NX + CET (IBT/SHSTK forcés, cf. qt6).
target_link_options(ncursessermo PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
  -Wl,-z,ibt -Wl,-z,shstk
  -fcf-protection=full -fstack-protector-strong -fstack-clash-protection)

target_link_libraries(ncursessermo PRIVATE
  ${SERMOCORE_LINK_LIBRARIES}     # cœur via pkg-config (variante neutre, lien statique)
  ${NCURSES_LIBRARIES})
target_link_directories(ncursessermo PRIVATE ${NCURSES_LIBRARY_DIRS})


# ─── Installation ─────────────────────────────────────────────────────────────
# Ces règles manquaient : seuls qt6 et le cœur en avaient. Qui construit depuis
# les sources n'avait aucun moyen d'installer le résultat — binaire, page de
# manuel et icônes étaient à recopier à la main, et COMPILE.md ne pouvait que
# désigner le binaire dans _build/. debian/rules pose ces mêmes fichiers de son
# côté : les deux chemins livrent désormais la même chose.
set(PACKAGE_VERSION "${PROJECT_VERSION}")
install(TARGETS ncursessermo DESTINATION bin)

# Ce port n'a pas de page propre : il reçoit la page générique du dépôt, sous
# son propre nom — exactement ce que fait debian/rules.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../doc/sermo.1.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ncursessermo.1"
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ncursessermo.1 DESTINATION share/man/man1)


# ─── Le port se nomme lui-même ────────────────────────────────────────────────
# « --version » est imprimé par le cœur, compilé une fois : il ne connaissait
# que son propre nom, et les sept ports s'annonçaient « sermocore ».
# contract/sermo_port_id.c remplit les deux symboles faibles que le cœur
# déclare, avec le nom et la version que CMake connaît déjà. La POSITION des
# mots ne bouge pas : seul le premier change (des scripts lisent le 3e).
target_sources(ncursessermo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract/sermo_port_id.c)
target_compile_definitions(ncursessermo PRIVATE
  SERMO_PORT_NAME="${PROJECT_NAME}"
  SERMO_PORT_VERSION="${PROJECT_VERSION}")
