# SPDX-License-Identifier: GPL-2.0-or-later
# sermo-backend-lvgl — backend de rendu LVGL (pilote X11), lié sur
# libsermocore (variante NEUTRE). Le CŒUR (parse/automaton/variables/actions/
# gtkdialog.c+main/lexer/parser) vit dans libsermocore ; ce backend ne garde que
# ses widgets (modèle arbre-de-WidgetNode partagé avec le port sdl3), le bridge
# d'attributs (tag_set_property.c), le hook d'init + l'ABI neutre
# (sermo_backend_lvgl.c) et le rendu/boucle LVGL (render_lvgl.c).
cmake_minimum_required(VERSION 3.16)
# Version : une seule source, le fichier VERSION à la racine du dépôt. Rien
# n'est à resynchroniser ici ; tests/garde_version.sh vérifie que tout le dépôt
# s'y accorde.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)

project(lvglsermo VERSION ${SERMO_VERSION} LANGUAGES C CXX)   # CXX : ThorVG (SVG), embarqué dans LVGL

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
pkg_check_modules(SERMOCORE REQUIRED sermocore)
pkg_check_modules(X11       REQUIRED x11)
pkg_check_modules(LIBPNG    REQUIRED libpng)   # --render-png
pkg_check_modules(WAYLAND   REQUIRED wayland-client wayland-cursor xkbcommon)
# Exigé à la configuration : sans lui, pkg_get_variable rend une chaîne vide et
# l'échec n'arrivait qu'à la compilation (« /stable/xdg-shell/xdg-shell.xml »).
pkg_check_modules(WL_PROTOS REQUIRED wayland-protocols)
pkg_get_variable(WL_PROTOCOLES wayland-protocols pkgdatadir)
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

# Le pilote Wayland de LVGL inclut des en-têtes de protocole qu'il ne fournit
# pas : on les engendre depuis wayland-protocols (wayland_<nom>.h/.c).
set(WL_GEN "${CMAKE_CURRENT_BINARY_DIR}/wayland")
file(MAKE_DIRECTORY ${WL_GEN})
set(WL_GEN_SRC)
# linux-dmabuf est passé de unstable/ à stable/ dans wayland-protocols 1.33, sous
# les mêmes noms d'interface (zwp_linux_dmabuf_v1). Slackware 15.0 n'a que la
# 1.25 : on y prend l'ancien chemin (mesuré le 12/10/2026, la construction
# s'arrêtait sur « No rule to make target …/stable/linux-dmabuf/… »).
if(EXISTS "${WL_PROTOCOLES}/stable/linux-dmabuf/linux-dmabuf-v1.xml")
  set(WL_DMABUF "stable/linux-dmabuf/linux-dmabuf-v1.xml")
else()
  set(WL_DMABUF "unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml")
endif()
foreach(paire "xdg_shell:stable/xdg-shell/xdg-shell.xml"
              "xdg_output:unstable/xdg-output/xdg-output-unstable-v1.xml"
              "linux_dmabuf:${WL_DMABUF}")
  string(REPLACE ":" ";" paire_l "${paire}")
  list(GET paire_l 0 nom)
  list(GET paire_l 1 xml)
  add_custom_command(
    OUTPUT ${WL_GEN}/wayland_${nom}.h ${WL_GEN}/wayland_${nom}.c
    COMMAND ${WAYLAND_SCANNER} client-header ${WL_PROTOCOLES}/${xml} ${WL_GEN}/wayland_${nom}.h
    COMMAND ${WAYLAND_SCANNER} private-code  ${WL_PROTOCOLES}/${xml} ${WL_GEN}/wayland_${nom}.c
    DEPENDS ${WL_PROTOCOLES}/${xml} VERBATIM)
  list(APPEND WL_GEN_SRC ${WL_GEN}/wayland_${nom}.c)
endforeach()

# ─── LVGL v9.6.0 embarqué ───────────────────────────────────────
# tiers/lvgl/ = src/ et include/ d'origine, sans retouche (voir PROVENANCE).
# Réglage : lv_conf.h à côté de ce fichier. Compilé à part, sans le shim GLib
# du port et sans avertissements (ce n'est pas notre code), mais durci comme lui.
set(LVGL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tiers/lvgl")
file(GLOB_RECURSE LVGL_SRC "${LVGL_DIR}/src/*.c" "${LVGL_DIR}/src/*.cpp")   # .cpp : ThorVG
add_library(lvgl_tiers STATIC ${LVGL_SRC} ${WL_GEN_SRC})
target_include_directories(lvgl_tiers PUBLIC ${LVGL_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(lvgl_tiers PRIVATE ${WL_GEN} ${WAYLAND_INCLUDE_DIRS})
target_compile_definitions(lvgl_tiers PUBLIC LV_CONF_INCLUDE_SIMPLE)
# Version légère (paquet sermo-backend-lvgl-leger) : sans ThorVG ni Lottie,
# donc sans icônes SVG ; ~31 % plus léger (mesuré le 15/10 : 1 663 Kio au lieu
# de 2 399, banc de comportement à parité). Même binaire « lvglsermo ».
option(SERMO_LVGL_LEGER "lvgl sans ThorVG/Lottie (icônes SVG absentes)" OFF)
if(SERMO_LVGL_LEGER)
  target_compile_definitions(lvgl_tiers PUBLIC SERMO_LVGL_LEGER)
endif()
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
target_compile_options(lvgl_tiers PRIVATE -w -fPIE
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection ${SERMO_FLOT_CONTROLE})

# Le pilote Wayland de LVGL crée ses tampons par shm_open/shm_unlink. Sans
# eux (bionic d'Android), on les fournit par memfd_create (sermo_shm_repli.c)
# et on les déclare à LVGL par un en-tête inclus de force, sans toucher à son
# code. glibc < 2.34 les range dans librt : essayé avant le repli.
include(CheckSymbolExists)
check_symbol_exists(shm_open "sys/mman.h" SERMO_A_SHM_OPEN)
if(NOT SERMO_A_SHM_OPEN)
  set(CMAKE_REQUIRED_LIBRARIES rt)
  check_symbol_exists(shm_open "sys/mman.h" SERMO_A_SHM_OPEN_RT)
  unset(CMAKE_REQUIRED_LIBRARIES)
endif()
set(SERMO_SHM_REPLI "")
if(SERMO_A_SHM_OPEN_RT)
  target_link_libraries(lvgl_tiers PUBLIC rt)
elseif(NOT SERMO_A_SHM_OPEN)
  target_compile_options(lvgl_tiers PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-include ${CMAKE_CURRENT_SOURCE_DIR}/src/sermo_shm_repli.h>)
  add_library(sermo_shm_repli OBJECT src/sermo_shm_repli.c)
  target_compile_options(sermo_shm_repli PRIVATE -Wall -Wextra -fPIE
    -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection ${SERMO_FLOT_CONTROLE})
  set(SERMO_SHM_REPLI $<TARGET_OBJECTS:sermo_shm_repli>)
endif()

# ─── Sources backend ───────────────────────────────────────────────────────
# widget*.c capture widget_*.c + widgets.c + widget_node.c + widget_stubs.c.
file(GLOB WIDGET_SRC "src/widget*.c")

# neutral_glist.c : fournit g_list_append EXTERNE au cœur (le shim lvgl-compat.h
# ne donne qu'un static inline, aucun symbole exporté). Compilé SANS le
# force-include du shim, sinon conflit de définition. Objet à part (cf. sdl3).
add_library(sermo_neutral_glist OBJECT src/neutral_glist.c)
target_compile_options(sermo_neutral_glist PRIVATE -fPIE)

add_executable(lvglsermo
  ${WIDGET_SRC}
  src/tag_set_property.c
  src/printing.c
  src/sermo_backend_lvgl.c
  src/render_lvgl.c
  $<TARGET_OBJECTS:sermo_neutral_glist>
  ${SERMO_SHM_REPLI}
)

target_include_directories(lvglsermo BEFORE PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}                  # config.h engendré
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract      # contrat sermo_be_* (MIT), rapatrie dans le depot
  ${CMAKE_CURRENT_SOURCE_DIR}/src        # en-têtes locaux (mêmes que le cœur neutre)
  ${SERMOCORE_INCLUDE_DIRS}
  ${X11_INCLUDE_DIRS}
  ${LIBPNG_INCLUDE_DIRS})

# Les objets à part sont liés dans le même binaire : un seul objet sans la
# protection du flot de contrôle et le lieur la retire du binaire entier
# (mesuré sur ARM64 avec -z bti-report). Même durcissement que le binaire.
target_compile_options(sermo_neutral_glist PRIVATE
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection ${SERMO_FLOT_CONTROLE})
target_compile_options(lvglsermo PRIVATE
  -std=gnu11
  # lvgl-compat.h : shim GLib auto-suffisant (GList, g_*), pas de glib réelle
  -include ${CMAKE_CURRENT_SOURCE_DIR}/src/lvgl-compat.h
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection ${SERMO_FLOT_CONTROLE}
  ${X11_CFLAGS_OTHER})

# Durcissement au lien : PIE/RELRO/BIND_NOW/NX + CET (IBT/SHSTK forcés, cf. qt6).
# Marquage CET (-z ibt/shstk) : x86 seulement, comme -fcf-protection.
if(SERMO_A_CF_PROTECTION)
  set(SERMO_CET_LIEN -Wl,-z,ibt -Wl,-z,shstk)
endif()
target_link_options(lvglsermo PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
  # XOpenDisplay du pilote X11 de LVGL : l'écran ouvert par le port, échec
  # sans plantage (src/render_lvgl.c, __wrap_XOpenDisplay)
  -Wl,--wrap=XOpenDisplay
  ${SERMO_CET_LIEN}
  ${SERMO_FLOT_CONTROLE} -fstack-protector-strong -fstack-clash-protection)

target_link_libraries(lvglsermo PRIVATE
  ${SERMOCORE_LINK_LIBRARIES}     # cœur via pkg-config (variante neutre, lien statique)
  lvgl_tiers
  ${X11_LIBRARIES}
  ${LIBPNG_LIBRARIES}
  ${WAYLAND_LIBRARIES}
  m)
target_link_directories(lvglsermo PRIVATE ${X11_LIBRARY_DIRS})
# Le pilote Wayland de LVGL appelle shm_open/shm_unlink : dans la libc depuis
# la glibc 2.34, dans librt avant (Slackware 15.0, glibc 2.33 : « undefined
# reference to shm_open », mesuré le 13/10/2026). Ne lier librt que là.
include(CheckSymbolExists)
check_symbol_exists(shm_open "sys/mman.h" SERMO_SHM_OPEN_DANS_LIBC)
if(NOT SERMO_SHM_OPEN_DANS_LIBC)
  target_link_libraries(lvglsermo PRIVATE rt)
endif()
# ThorVG (C++) est dans lvgl_tiers : l'édition des liens se fait en C++
# (libstdc++), le reste du port reste du C.
set_target_properties(lvglsermo PROPERTIES LINKER_LANGUAGE CXX)


# ─── Installation ─────────────────────────────────────────────────────────────
# Ces règles manquaient : seuls qt6 et le cœur en avaient. Qui construit depuis
# les sources n'avait aucun moyen d'installer le résultat — binaire, page de
# manuel et icônes étaient à recopier à la main, et COMPILE.md ne pouvait que
# désigner le binaire dans _build/. debian/rules pose ces mêmes fichiers de son
# côté : les deux chemins livrent désormais la même chose.
set(PACKAGE_VERSION "${PROJECT_VERSION}")
install(TARGETS lvglsermo DESTINATION bin)

# Ce port n'a pas de page propre : il reçoit la page générique du dépôt, sous
# son propre nom — exactement ce que fait debian/rules.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../doc/sermo.1.in"
  "${CMAKE_CURRENT_BINARY_DIR}/lvglsermo.1"
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lvglsermo.1 DESTINATION share/man/man1)


# ─── Le port se nomme lui-même ────────────────────────────────────────────────
# « --version » est imprimé par le cœur, compilé une fois : il ne connaissait
# que son propre nom, et les sept ports s'annonçaient « sermocore ».
# contract/sermo_port_id.c remplit les deux symboles faibles que le cœur
# déclare, avec le nom et la version que CMake connaît déjà. La POSITION des
# mots ne bouge pas : seul le premier change (des scripts lisent le 3e).
target_sources(lvglsermo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract/sermo_port_id.c)
target_compile_definitions(lvglsermo PRIVATE
  SERMO_PORT_NAME="${PROJECT_NAME}"
  SERMO_PORT_VERSION="${PROJECT_VERSION}")
