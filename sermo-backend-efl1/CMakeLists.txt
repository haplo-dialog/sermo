# SPDX-License-Identifier: GPL-2.0-or-later
# sermo-backend-efl1 — backend de rendu EFL/Elementary, lié sur libsermocore (variante NEUTRE).
# Preuve 1.5 : le MÊME cœur (source unifiée, variante neutre) sert la
# famille EFL. Le cœur (parse/exec/automaton/attributs/actions/main) vit dans
# libsermocore ; ce backend ne garde que ses widgets + le bridge d'attributs
# (tag_set_property.c) + le hook d'init toolkit (sermo_backend_efl1sermo.c).
cmake_minimum_required(VERSION 3.16)
# Version : une seule source, le fichier VERSION à la racine du dépôt. Rien
# n'est à resynchroniser ici ; tests/garde_version.sh vérifie que tout le dépôt
# s'y accorde.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)

project(efl1sermo VERSION ${SERMO_VERSION} LANGUAGES C)

# FORTIFY_SOURCE n'agit qu'avec l'optimisation : sans type de build, CMake ne
# passe aucun -O et la protection promise par SECURITY.md reste lettre morte.
# Même règle que qt6, sdl3 et ncurses. Vérifié par tests/garde_fortify_coeur.sh
# (cœur) et tests/garde_durcissement.sh (binaires).
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

find_package(PkgConfig REQUIRED)

# ─── Toolkit EFL/Elementary ──────────────────────────────────────────────────
pkg_check_modules(EFL       REQUIRED elementary ecore-file)
pkg_check_modules(ECORE     REQUIRED ecore)
pkg_check_modules(ECORE_EXE ecore-exe)          # terminal, optionnel
pkg_check_modules(EDJE      edje)               # thèmes EDC, optionnel
pkg_check_modules(GLIB      REQUIRED glib-2.0)  # HAVE_GLIB : le cœur est du code GLib

if(ECORE_EXE_FOUND)
  set(HAVE_ECORE_EXE 1)
else()
  set(HAVE_ECORE_EXE 0)
endif()

# ─── Cœur consommé via pkg-config (variante NEUTRE) ──────────────────────────
pkg_check_modules(SERMOCORE REQUIRED sermocore)

# ─── Sources backend ─────────────────────────────────────────────────────────
file(GLOB WIDGET_SRC "src/widget*.c")   # widget_*.c ET widgets.c (glue de rendu)
set(BACKEND_SRC
  ${WIDGET_SRC}
  src/efl-globals.c
  src/efl_neutral_abi.c               # ABI neutre cœur→backend (qt6_* → EFL)
  src/tag_set_property.c              # bridge attributs→widgets (sorti du cœur)
  src/sermo_backend_efl1sermo.c       # hook d'init toolkit (carve 1.2) + globales de visibilité
)

add_executable(efl1sermo ${BACKEND_SRC})

target_include_directories(efl1sermo PRIVATE
  src/
  ${SERMOCORE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract      # contrat sermo_be_* (MIT), rapatrie dans le depot
  ${EFL_INCLUDE_DIRS} ${ECORE_INCLUDE_DIRS} ${ECORE_EXE_INCLUDE_DIRS}
  ${EDJE_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})

target_compile_options(efl1sermo PRIVATE
  -std=gnu11
  ${EFL_CFLAGS_OTHER} ${GLIB_CFLAGS_OTHER}
  -DHAVE_GLIB=1                       # GList/g_* réels ; ABI identique au cœur neutre
  -DHAVE_ECORE_EXE=${HAVE_ECORE_EXE}
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection -fcf-protection=full
  # Force-include du header de compat du port sur chaque unité C (comme le port amont).
  -include ${CMAKE_CURRENT_SOURCE_DIR}/src/efl-compat.h)

# Durcissement au lien : PIE/RELRO/BIND_NOW/NX + CET (IBT/SHSTK forcés, cf. qt6).
# Vérifié par tests/garde_durcissement.sh.
target_link_options(efl1sermo PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
  -Wl,-z,ibt -Wl,-z,shstk
  -fcf-protection=full -fstack-protector-strong -fstack-clash-protection)

target_link_libraries(efl1sermo
  ${SERMOCORE_LINK_LIBRARIES}         # cœur via pkg-config (variante neutre, lien statique)
  ${EFL_LIBRARIES} ${ECORE_LIBRARIES} ${ECORE_EXE_LIBRARIES}
  ${EDJE_LIBRARIES} ${GLIB_LIBRARIES})
target_link_directories(efl1sermo PRIVATE
  ${EFL_LIBRARY_DIRS} ${ECORE_LIBRARY_DIRS} ${ECORE_EXE_LIBRARY_DIRS}
  ${EDJE_LIBRARY_DIRS} ${GLIB_LIBRARY_DIRS})


# ─── Installation ─────────────────────────────────────────────────────────────
# Ces règles manquaient : seuls qt6 et le cœur en avaient. Qui construit depuis
# les sources n'avait aucun moyen d'installer le résultat — binaire, page de
# manuel et icônes étaient à recopier à la main, et COMPILE.md ne pouvait que
# désigner le binaire dans _build/. debian/rules pose ces mêmes fichiers de son
# côté : les deux chemins livrent désormais la même chose.
set(PACKAGE_VERSION "${PROJECT_VERSION}")
install(TARGETS efl1sermo DESTINATION bin)

# Ce port n'a pas de page propre : il reçoit la page générique du dépôt, sous
# son propre nom — exactement ce que fait debian/rules.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../doc/sermo.1.in"
  "${CMAKE_CURRENT_BINARY_DIR}/efl1sermo.1"
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/efl1sermo.1 DESTINATION share/man/man1)

# Icône d'application, aux huit tailles du thème hicolor. OPTIONAL : une taille
# absente ne doit pas faire échouer l'installation.
foreach(taille 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256)
  install(FILES data/icons/hicolor/${taille}/apps/efl1sermo.png
    DESTINATION share/icons/hicolor/${taille}/apps
    OPTIONAL)
endforeach()


# ─── Le port se nomme lui-même ────────────────────────────────────────────────
# « --version » est imprimé par le cœur, compilé une fois : il ne connaissait
# que son propre nom, et les sept ports s'annonçaient « sermocore ».
# contract/sermo_port_id.c remplit les deux symboles faibles que le cœur
# déclare, avec le nom et la version que CMake connaît déjà. La POSITION des
# mots ne bouge pas : seul le premier change (des scripts lisent le 3e).
target_sources(efl1sermo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract/sermo_port_id.c)
target_compile_definitions(efl1sermo PRIVATE
  SERMO_PORT_NAME="${PROJECT_NAME}"
  SERMO_PORT_VERSION="${PROJECT_VERSION}")
