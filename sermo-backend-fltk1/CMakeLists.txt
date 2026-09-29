# SPDX-License-Identifier: GPL-2.0-or-later
# sermo-backend-fltk1 — backend de rendu FLTK 1.4 (C++), lié sur libsermocore
# (variante NEUTRE). Preuve : le MÊME cœur (source unifiée)
# sert aussi une famille non-GObject (FLTK), comme qt6.
#
# Motif identique à sermo-backend-qt6 / sermo-backend-gtk3 :
#   - le CŒUR (parse/automaton/variables/actions/gtkdialog.c+main/…) vient de
#     libsermocore via pkg-config ;
#   - ce backend ne garde que ses widgets FLTK + le bridge d'attributs
#     (tag_set_property.c) + le hook d'init toolkit (sermo_backend_fltk1sermo.c).
#
# Le port fltk1 est bâti autour de la vraie GLib (HAVE_GLIB=1 : GList/g_malloc/
# g_strdup viennent de <glib.h>) et de FLTK ; on réplique EXACTEMENT cet
# environnement de compilation (mêmes -D, même force-include fltk-compat.h) pour
# que les unités se compilent comme dans le port. Le seul symbole g_* laissé
# indéfini par le cœur neutre (g_list_append) est résolu par la vraie GLib
# (ABI GList identique : {data,next,prev}).
cmake_minimum_required(VERSION 3.16)
# Version : une seule source, le fichier VERSION à la racine du dépôt. Rien
# n'est à resynchroniser ici ; tests/garde_version.sh vérifie que tout le dépôt
# s'y accorde.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)

project(fltk1sermo VERSION ${SERMO_VERSION} LANGUAGES C CXX)

# FORTIFY_SOURCE n'agit qu'avec l'optimisation : sans type de build, CMake ne
# passe aucun -O et la protection promise par SECURITY.md reste lettre morte.
# Même règle que qt6, sdl3 et ncurses. Vérifié par tests/garde_fortify_coeur.sh
# (cœur) et tests/garde_durcissement.sh (binaires).
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
# 1.3 : cœur consommé via pkg-config (variante NEUTRE)
pkg_check_modules(SERMOCORE REQUIRED sermocore)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# ─── FLTK 1.4 via fltk-config (pas de pkg-config pour FLTK) ──────────────────
find_program(FLTK_CONFIG fltk-config REQUIRED)
execute_process(COMMAND ${FLTK_CONFIG} --cxxflags
  OUTPUT_VARIABLE FLTK_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${FLTK_CONFIG} --ldflags --use-images
  OUTPUT_VARIABLE FLTK_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(FLTK_CXXFLAGS_LIST UNIX_COMMAND "${FLTK_CXXFLAGS}")
separate_arguments(FLTK_LDFLAGS_LIST  UNIX_COMMAND "${FLTK_LDFLAGS}")

# Fl_Terminal.H présent dans FLTK 1.4 → widget <terminal> réel.
find_file(FL_TERMINAL_H FL/Fl_Terminal.H)
if(FL_TERMINAL_H)
  set(HAVE_FL_TERMINAL 1)
else()
  set(HAVE_FL_TERMINAL 0)
endif()

# ─── Sources backend ─────────────────────────────────────────────────────────
# Tous les widget*.c/.cpp (widgets.cpp = glue de rendu, définit aussi
# widget_show_list/hide_list) + fltk-compat.cpp + le bridge + le hook toolkit.
file(GLOB WIDGET_SRC CONFIGURE_DEPENDS "src/widget*.cpp")
set(BACKEND_SRC
  ${WIDGET_SRC}
  src/fltk-compat.cpp
  src/tag_set_property.c            # bridge attributs (sorti du cœur)
  src/sermo_backend_fltk1sermo.c    # hook d'init toolkit (carve 1.2)
  src/neutral_toolkit_bridge.c      # ABI neutre qt6_* → ponts FLTK (fltk-compat)
)

add_executable(fltk1sermo ${BACKEND_SRC})

target_include_directories(fltk1sermo PRIVATE
  src/
  ${SERMOCORE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract      # contrat sermo_be_* (MIT), rapatrie dans le depot
  ${GLIB_INCLUDE_DIRS})

# Force-include fltk-compat.h sur CHAQUE unité (C et C++), comme le port
# (AM_CFLAGS/AM_CXXFLAGS).
target_compile_options(fltk1sermo PRIVATE
  ${FLTK_CXXFLAGS_LIST}
  ${GLIB_CFLAGS_OTHER}
  -DHAVE_FL_TERMINAL=${HAVE_FL_TERMINAL}
  -DHAVE_GLIB=1
  -D_POSIX_C_SOURCE=200809L
  -include ${CMAKE_CURRENT_SOURCE_DIR}/src/fltk-compat.h
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection
  -fcf-protection=full)

# Durcissement au lien : PIE/RELRO/BIND_NOW/NX + CET (IBT/SHSTK forcés, cf. qt6).
# Vérifié par tests/garde_durcissement.sh.
target_link_options(fltk1sermo PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
  -Wl,-z,ibt -Wl,-z,shstk
  -fcf-protection=full -fstack-protector-strong -fstack-clash-protection)

target_link_libraries(fltk1sermo
  ${SERMOCORE_LINK_LIBRARIES}   # cœur via pkg-config (variante neutre, lien statique)
  ${FLTK_LDFLAGS_LIST}
  ${GLIB_LIBRARIES})


# ─── Installation ─────────────────────────────────────────────────────────────
# Ces règles manquaient : seuls qt6 et le cœur en avaient. Qui construit depuis
# les sources n'avait aucun moyen d'installer le résultat — binaire, page de
# manuel et icônes étaient à recopier à la main, et COMPILE.md ne pouvait que
# désigner le binaire dans _build/. debian/rules pose ces mêmes fichiers de son
# côté : les deux chemins livrent désormais la même chose.
set(PACKAGE_VERSION "${PROJECT_VERSION}")
install(TARGETS fltk1sermo DESTINATION bin)

# Ce port n'a pas de page propre : il reçoit la page générique du dépôt, sous
# son propre nom — exactement ce que fait debian/rules.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../doc/sermo.1.in"
  "${CMAKE_CURRENT_BINARY_DIR}/fltk1sermo.1"
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fltk1sermo.1 DESTINATION share/man/man1)

# Icône d'application, aux huit tailles du thème hicolor. OPTIONAL : une taille
# absente ne doit pas faire échouer l'installation.
foreach(taille 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256)
  install(FILES data/icons/hicolor/${taille}/apps/fltk1sermo.png
    DESTINATION share/icons/hicolor/${taille}/apps
    OPTIONAL)
endforeach()


# ─── Le port se nomme lui-même ────────────────────────────────────────────────
# « --version » est imprimé par le cœur, compilé une fois : il ne connaissait
# que son propre nom, et les sept ports s'annonçaient « sermocore ».
# contract/sermo_port_id.c remplit les deux symboles faibles que le cœur
# déclare, avec le nom et la version que CMake connaît déjà. La POSITION des
# mots ne bouge pas : seul le premier change (des scripts lisent le 3e).
target_sources(fltk1sermo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract/sermo_port_id.c)
target_compile_definitions(fltk1sermo PRIVATE
  SERMO_PORT_NAME="${PROJECT_NAME}"
  SERMO_PORT_VERSION="${PROJECT_VERSION}")
