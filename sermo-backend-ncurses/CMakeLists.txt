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

# Le lien se fait en -pie : le code doit être compilé en -fPIE. Debian le fait par
# défaut, pas toutes les distributions (Slackware 15 : « relocation R_X86_64_32 …
# recompile with -fPIE », mesuré le 2026-10-10).
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
if(WIN32)
  # Windows : tout en statique, le .exe ne dépend que de DLL du
  # système. Sans ceci, « -lpthread » se résout en libpthread.dll.a et le
  # programme exige libwinpthread-1.dll (mesuré le 2026-10-11).
  # Posé ICI : la plateforme Windows de CMake réécrit ce réglage après le
  # fichier de chaîne.
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()
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

# Protection du flot de contrôle selon le processeur : CET sur x86
# (-fcf-protection), BTI/PAC sur ARM64 (-mbranch-protection). Écrite en dur,
# -fcf-protection arrêtait toute construction ARM64 : « option 'cf-protection'
# cannot be specified on this target » (clang 21, Termux, mesuré le 2026-10-12).
include(CheckCCompilerFlag)
check_c_compiler_flag(-fcf-protection=full SERMO_A_CF_PROTECTION)
check_c_compiler_flag(-mbranch-protection=standard SERMO_A_BRANCH_PROTECTION)
if(SERMO_A_CF_PROTECTION)
  set(SERMO_FLOT_CONTROLE -fcf-protection=full)
elseif(SERMO_A_BRANCH_PROTECTION)
  set(SERMO_FLOT_CONTROLE -mbranch-protection=standard)
else()
  set(SERMO_FLOT_CONTROLE "")
endif()
# -fstack-clash-protection : gardé si le compilateur l'accepte (clang d'OpenBSD
# l'ignorait avec un avertissement par fichier ; celui d'Apple peut le refuser).
# clang l'accepte à l'essai puis l'ignore fichier par fichier (« argument
# unused », 79 fois sous macOS le 2026-10-13) : chez lui, cet avertissement
# compte comme un refus.
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(CMAKE_REQUIRED_FLAGS "-Werror=unused-command-line-argument")
endif()
check_c_compiler_flag(-fstack-clash-protection SERMO_A_STACK_CLASH)
unset(CMAKE_REQUIRED_FLAGS)
if(SERMO_A_STACK_CLASH)
  set(SERMO_PILE_CLASH -fstack-clash-protection)
else()
  set(SERMO_PILE_CLASH "")
endif()
# strtod_l (nombres d'un XML lus en locale C) : glibc, musl, FreeBSD et macOS
# (par <xlocale.h>) l'ont, OpenBSD non. Sans lui, les shims passent par
# uselocale (SERMO_SANS_STRTOD_L). Essayé, pas deviné.
include(CheckIncludeFile)
include(CheckSymbolExists)
check_include_file(xlocale.h SERMO_A_XLOCALE_H)
set(_sermo_entetes_strtod "stdlib.h;locale.h")
if(SERMO_A_XLOCALE_H)
  list(APPEND _sermo_entetes_strtod "xlocale.h")
endif()
set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(strtod_l "${_sermo_entetes_strtod}" SERMO_A_STRTOD_L)
unset(CMAKE_REQUIRED_DEFINITIONS)
if(NOT SERMO_A_STRTOD_L AND NOT WIN32)
  target_compile_definitions(ncursessermo PRIVATE SERMO_SANS_STRTOD_L=1)
endif()
if(APPLE)
  # Extensions de macOS (strtod_l…) malgré _POSIX_C_SOURCE.
  target_compile_definitions(ncursessermo PRIVATE _DARWIN_C_SOURCE=1)
endif()

# Les objets à part sont liés dans le même binaire : un seul objet sans la
# protection du flot de contrôle et le lieur la retire du binaire entier
# (mesuré sur ARM64 avec -z bti-report). Même durcissement que le binaire.
target_compile_options(sermo_neutral_glist PRIVATE
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection ${SERMO_FLOT_CONTROLE})
target_compile_options(ncursessermo PRIVATE
  -std=gnu11
  # ncurses-compat.h : shim GLib auto-suffisant (GList, g_*), pas de glib réelle
  -include ${CMAKE_CURRENT_SOURCE_DIR}/src/ncurses-compat.h
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong ${SERMO_PILE_CLASH} ${SERMO_FLOT_CONTROLE}
  ${NCURSES_CFLAGS_OTHER})

# Durcissement au lien : PIE/RELRO/BIND_NOW/NX + CET (IBT/SHSTK forcés, cf. qt6).
if(WIN32)
  # Windows : les options -z sont propres à ELF. Équivalents PE :
  # ASLR haute entropie et DEP, que mingw-w64 pose par défaut ; on les exige
  # quand même. -lssp : le protecteur de pile de mingw vit à part. Lien
  # statique de libgcc : le .exe ne dépend que de DLL du système.
  target_link_options(ncursessermo PRIVATE
    -Wl,--dynamicbase -Wl,--high-entropy-va -Wl,--nxcompat
    -fstack-protector-strong -static -static-libgcc)
  target_link_libraries(ncursessermo PRIVATE ssp)
  # Manifeste : page de code UTF-8 pour tout le programme (environnement,
  # chemins accentués), Windows 10/11 déclarés.
  enable_language(RC)
  target_sources(ncursessermo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../packaging/autres-os/windows/sermo.rc)
elseif(APPLE)
  # macOS : le lieur d'Apple ne connaît pas -z (RELRO n'existe pas en Mach-O) ;
  # PIE et pile non exécutable y sont le défaut.
  target_link_options(ncursessermo PRIVATE
    ${SERMO_FLOT_CONTROLE} -fstack-protector-strong ${SERMO_PILE_CLASH})
else()
  # Marquage CET (-z ibt/shstk) : x86 seulement, comme -fcf-protection.
  if(SERMO_A_CF_PROTECTION)
    set(SERMO_CET_LIEN -Wl,-z,ibt -Wl,-z,shstk)
  endif()
  target_link_options(ncursessermo PRIVATE
    -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
    ${SERMO_CET_LIEN}
    ${SERMO_FLOT_CONTROLE} -fstack-protector-strong ${SERMO_PILE_CLASH})
endif()

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
