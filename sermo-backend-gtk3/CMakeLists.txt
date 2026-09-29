# SPDX-License-Identifier: GPL-2.0-or-later
# sermo-backend-gtk3 — backend de rendu GTK3, lié sur libsermocore (variante gtk).
# Preuve 1.4 : le MÊME cœur (source unifiée) sert la famille GObject.
cmake_minimum_required(VERSION 3.16)
# Version : une seule source, le fichier VERSION à la racine du dépôt. Rien
# n'est à resynchroniser ici ; tests/garde_version.sh vérifie que tout le dépôt
# s'y accorde.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)

project(gtk3sermo VERSION ${SERMO_VERSION} LANGUAGES C)

# FORTIFY_SOURCE n'agit qu'avec l'optimisation : sans type de build, CMake ne
# passe aucun -O et la protection promise par SECURITY.md reste lettre morte.
# Même règle que qt6, sdl3 et ncurses. Vérifié par tests/garde_fortify_coeur.sh
# (cœur) et tests/garde_durcissement.sh (binaires).
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0 glib-2.0)

# 1.3 : cœur consommé via pkg-config (variante gtk)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SERMOCORE REQUIRED sermocore)

# <terminal> : le cœur a tranché (variable « vte » de sermocore.pc) et le backend
# suit. Un cœur avec VTE et un backend sans, ou l'inverse, ne s'entendraient pas
# sur ce qu'est le widget.
pkg_get_variable(SERMOCORE_VTE sermocore vte)
if(SERMOCORE_VTE STREQUAL "1")
  pkg_check_modules(VTE REQUIRED vte-2.91)
  set(HAVE_VTE 1)
  message(STATUS "gtk3sermo : <terminal> par vte-2.91 ${VTE_VERSION}")
else()
  set(HAVE_VTE 0)
  message(WARNING "gtk3sermo : cœur construit sans VTE — <terminal> n'affichera qu'un avertissement")
endif()

# Ancrage Wayland (wlr-layer-shell) : attributs layer, edge, dist et reserve de
# <window>. ⚠️ Même piège que VTE : le -DHAVE_LAYER_SHELL ne vivait que dans
# l'ancien Makefile.am, et la fonction a manqué de la 2.0 à la 2.7.0 sans un mot.
# Absent, gtk-layer-shell arrête donc la construction, sauf refus explicite.
option(SERMO_LAYER_SHELL "Ancrage Wayland des fenêtres (gtk-layer-shell)" ON)
set(HAVE_LAYER_SHELL 0)
set(HAVE_LAYER_SHELL_IS_SUPPORTED 0)
if(SERMO_LAYER_SHELL)
  pkg_check_modules(LAYER_SHELL gtk-layer-shell-0)
  if(NOT LAYER_SHELL_FOUND)
    message(FATAL_ERROR "gtk3sermo : gtk-layer-shell-0 introuvable. Installer libgtk-layer-shell-dev, "
      "ou construire explicitement sans : -DSERMO_LAYER_SHELL=OFF.")
  endif()
  set(HAVE_LAYER_SHELL 1)
  # gtk_layer_is_supported() existe depuis gtk-layer-shell 0.5.
  if(LAYER_SHELL_VERSION VERSION_GREATER_EQUAL 0.5.0)
    set(HAVE_LAYER_SHELL_IS_SUPPORTED 1)
  endif()
  message(STATUS "gtk3sermo : ancrage Wayland par gtk-layer-shell-0 ${LAYER_SHELL_VERSION}")
else()
  message(WARNING "gtk3sermo : construit SANS layer-shell — layer, edge, dist et reserve seront ignorés")
endif()

file(GLOB WIDGET_SRC "src/widget*.c")   # widget_*.c ET widgets.c (glue de rendu)
set(BACKEND_SRC
  ${WIDGET_SRC}
  src/glade_support.c
  src/printing.c
  src/tag_set_property.c        # bridge GObject (sorti du cœur)
  src/sermo_backend_gtk3.c      # hook d'init toolkit (carve 1.2)
)

add_executable(gtk3sermo ${BACKEND_SRC})

target_include_directories(gtk3sermo PRIVATE
  src/
  ${SERMOCORE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract      # contrat sermo_be_* (MIT), rapatrie dans le depot
  ${GTK3_INCLUDE_DIRS} ${VTE_INCLUDE_DIRS} ${LAYER_SHELL_INCLUDE_DIRS})

target_compile_options(gtk3sermo PRIVATE
  -std=gnu11 ${GTK3_CFLAGS_OTHER}
  -DHAVE_VTE=${HAVE_VTE}
  -DHAVE_LAYER_SHELL=${HAVE_LAYER_SHELL} -DHAVE_LAYER_SHELL_IS_SUPPORTED=${HAVE_LAYER_SHELL_IS_SUPPORTED}
  -Wno-deprecated-declarations   # appels GTK 3 dépréciés : GTK 3 est gelé
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection -fcf-protection=full)

# Durcissement au lien : PIE/RELRO/BIND_NOW/NX + CET (IBT/SHSTK forcés — les crt
# de la glibc testing ne portent plus la note CET et la fusion du lieur est un ET,
# donc on la force ici, comme qt6). Vérifié par tests/garde_durcissement.sh.
target_link_options(gtk3sermo PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
  -Wl,-z,ibt -Wl,-z,shstk
  -fcf-protection=full -fstack-protector-strong -fstack-clash-protection)

target_link_libraries(gtk3sermo
  ${SERMOCORE_LINK_LIBRARIES}   # cœur via pkg-config (variante gtk, lien statique)
  ${GTK3_LIBRARIES} ${VTE_LIBRARIES} ${LAYER_SHELL_LIBRARIES})
target_link_directories(gtk3sermo PRIVATE ${GTK3_LIBRARY_DIRS} ${VTE_LIBRARY_DIRS} ${LAYER_SHELL_LIBRARY_DIRS})


# ─── Installation ─────────────────────────────────────────────────────────────
# Ces règles manquaient : seuls qt6 et le cœur en avaient. Qui construit depuis
# les sources n'avait aucun moyen d'installer le résultat — binaire, page de
# manuel et icônes étaient à recopier à la main, et COMPILE.md ne pouvait que
# désigner le binaire dans _build/. debian/rules pose ces mêmes fichiers de son
# côté : les deux chemins livrent désormais la même chose.
set(PACKAGE_VERSION "${PROJECT_VERSION}")
install(TARGETS gtk3sermo DESTINATION bin)

# Page de manuel propre au port, substituée depuis son modèle.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/gtk3sermo.1.in"
  "${CMAKE_CURRENT_BINARY_DIR}/gtk3sermo.1"
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gtk3sermo.1 DESTINATION share/man/man1)

# Page de section 5 : le format XML.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/haplo-dialog-xml.5.in"
  "${CMAKE_CURRENT_BINARY_DIR}/haplo-dialog-xml.5"
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/haplo-dialog-xml.5 DESTINATION share/man/man5)

# Icône d'application, aux huit tailles du thème hicolor. OPTIONAL : une taille
# absente ne doit pas faire échouer l'installation.
foreach(taille 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256)
  install(FILES data/icons/hicolor/${taille}/apps/gtk3sermo.png
    DESTINATION share/icons/hicolor/${taille}/apps
    OPTIONAL)
endforeach()


# ─── Le port se nomme lui-même ────────────────────────────────────────────────
# « --version » est imprimé par le cœur, compilé une fois : il ne connaissait
# que son propre nom, et les sept ports s'annonçaient « sermocore ».
# contract/sermo_port_id.c remplit les deux symboles faibles que le cœur
# déclare, avec le nom et la version que CMake connaît déjà. La POSITION des
# mots ne bouge pas : seul le premier change (des scripts lisent le 3e).
target_sources(gtk3sermo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract/sermo_port_id.c)
target_compile_definitions(gtk3sermo PRIVATE
  SERMO_PORT_NAME="${PROJECT_NAME}"
  SERMO_PORT_VERSION="${PROJECT_VERSION}")
