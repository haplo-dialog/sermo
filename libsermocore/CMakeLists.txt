# SPDX-License-Identifier: GPL-2.0-or-later
# libsermocore — cœur syntaxique + exécution durcie de sermo.
# UNE source unifiée, plusieurs variantes de build :
#   - neutre (défaut)   : shim + stubs, aucun toolkit → qt6/fltk1/efl1/sdl3.
#   - SERMOCORE_GLIB=ON : vraie GLib/GTK3 (ABI native)  → gtk3.
#   - SERMOCORE_GTK4=ON : variante GTK4 « voie (b) » — fichiers cœur RÉÉCRITS pour
#     GTK4, VENDORÉS dans src-gtk4/ (plus aucune dépendance externe).
#     GTK4 a retiré gtk_main/gtk_socket/le modèle d'évts ; variables.c/automaton.c/
#     signals.c/actions.c/gtkdialog.c divergent trop pour la source unifiée.
#     Grammaire partagée. La source unique (Part 3) reste à faire.
cmake_minimum_required(VERSION 3.16)
# Version : une seule source, le fichier VERSION à la racine du dépôt. Rien
# n'est à resynchroniser ici ; tests/garde_version.sh vérifie que tout le dépôt
# s'y accorde.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)

project(sermocore VERSION ${SERMO_VERSION} LANGUAGES C)

# FORTIFY_SOURCE n'agit qu'avec l'optimisation : sans type de build, CMake ne
# passe aucun -O et la protection promise par SECURITY.md reste lettre morte.
# Même règle que qt6, sdl3 et ncurses. Vérifié par tests/garde_fortify_coeur.sh
# (cœur) et tests/garde_durcissement.sh (binaires).
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
set(SERMOCORE_VERSION ${PROJECT_VERSION})
# Où s'installent la bibliothèque et sermocore.pc, relativement au préfixe.
# « lib » par défaut (ci/construire.sh et COMPILE.md s'y fient) ; le paquet
# Debian passe le répertoire multiarch (lib/x86_64-linux-gnu…), où un fichier
# pkg-config dépendant de l'architecture doit vivre.
set(SERMOCORE_LIBDIR "lib" CACHE STRING "Répertoire de libsermocore.a et de sermocore.pc sous le préfixe")

option(SERMOCORE_GLIB "Compiler le cœur avec la vraie GLib/GTK3 (famille gtk3)" OFF)
option(SERMOCORE_GTK4 "Variante GTK4 (fichiers cœur de gtk4sermo)" OFF)
# Widget <terminal> des variantes GLib et GTK4 : VTE. Le choix se fait ICI et
# part dans sermocore.pc (variable « vte ») ; les backends gtk3 et gtk4 le
# suivent, pour que le cœur et le backend compilent le MÊME widget.
# ⚠️ PIÈGE VÉCU (2.0 → 2.7.0) : VTE était « optionnel » et le -DHAVE_VTE ne
# vivait que dans les anciens Makefile.am ; sous CMake le terminal a disparu
# sans un message. Absent, VTE arrête donc la construction, sauf refus explicite.
option(SERMOCORE_VTE "Widget <terminal> par VTE (variantes GLib et GTK4)" ON)
set(SERMOCORE_VTE_PC 0)

# Cœur variante GTK4 vendoré dans le dépôt — plus aucune dépendance externe.
set(GTK4SRC "${CMAKE_CURRENT_SOURCE_DIR}/src-gtk4")

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
FLEX_TARGET(SermoLexer  src/gtkdialog_lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_lexer.c
  COMPILE_FLAGS "-P gtkdialog_"
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_lexer.h)
BISON_TARGET(SermoParser src/gtkdialog_parser.y ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_parser.c
  COMPILE_FLAGS "-p gtkdialog_"
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_parser.h)
ADD_FLEX_BISON_DEPENDENCY(SermoLexer SermoParser)

if(SERMOCORE_GTK4)
  # Fichiers cœur encore PROPRES à GTK4 (vraies différences d'API GTK3/GTK4,
  # fusion sous #ifdef à venir — étape 3b) + gtk4-compat.c.
  set(CORE_SRC
    ${GTK4SRC}/gtkdialog.c ${GTK4SRC}/variables.c ${GTK4SRC}/automaton.c
    ${GTK4SRC}/signals.c ${GTK4SRC}/tag_attributes.c
    ${GTK4SRC}/gtk4-compat.c
    # Fichiers cœur à SOURCE UNIQUE (src/), valables pour toutes les variantes :
    # l'écart gtk3d.h↔gtkdialog.h est ponté par le shim src-gtk4/gtk3d.h, et la
    # journalisation g_* est disponible en GTK4. (étapes 3a-3b)
    ${CMAKE_CURRENT_SOURCE_DIR}/src/actions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/safe_exec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stack.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/attributes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stringman.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sermo_icon_theme.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sermo_input.c
    ${FLEX_SermoLexer_OUTPUTS} ${BISON_SermoParser_OUTPUTS})
else()
  set(CORE_SRC
    src/gtkdialog.c
    src/safe_exec.c src/variables.c src/stack.c src/automaton.c src/attributes.c
    src/signals.c src/stringman.c src/actions.c src/tag_attributes.c src/sermo_icon_theme.c
    src/sermo_input.c
    ${FLEX_SermoLexer_OUTPUTS} ${BISON_SermoParser_OUTPUTS})
  if(NOT SERMOCORE_GLIB)
    # Barre de progression lue au fil de l'eau, pour les ports sans GTK
    # (qt6, fltk1, efl1, sdl3, ncurses) ; gtk3 garde son fil de lecture.
    list(APPEND CORE_SRC src/sermo_progress.c)
  endif()
endif()

add_library(sermocore STATIC ${CORE_SRC})

# VTE pour <terminal> (variantes GLib et GTK4) — voir l'option SERMOCORE_VTE.
function(sermocore_vte module paquet)
  if(NOT SERMOCORE_VTE)
    target_compile_definitions(sermocore PRIVATE HAVE_VTE=0)
    message(WARNING "sermocore : construit SANS VTE (-DSERMOCORE_VTE=OFF) — le widget <terminal> n'affichera qu'un avertissement")
    return()
  endif()
  pkg_check_modules(VTE ${module})
  if(NOT VTE_FOUND)
    message(FATAL_ERROR "sermocore : ${module} introuvable, le widget <terminal> ne pourrait pas exister. "
      "Installer ${paquet}, ou construire explicitement sans : -DSERMOCORE_VTE=OFF.")
  endif()
  target_include_directories(sermocore PUBLIC ${VTE_INCLUDE_DIRS})
  target_compile_definitions(sermocore PRIVATE HAVE_VTE=1)
  set(SERMOCORE_VTE_PC 1 PARENT_SCOPE)
  message(STATUS "sermocore : <terminal> par ${module} ${VTE_VERSION}")
endfunction()
set_target_properties(sermocore PROPERTIES OUTPUT_NAME sermocore POSITION_INDEPENDENT_CODE ON)
target_include_directories(sermocore PUBLIC include ${CMAKE_CURRENT_BINARY_DIR})
# Une seule source pour le numéro de version : project(VERSION) ci-dessus.
target_compile_definitions(sermocore PRIVATE PACKAGE_VERSION="${PROJECT_VERSION}")
target_compile_options(sermocore PRIVATE
  -std=gnu11
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection -fcf-protection=full
  -fPIC)

if(SERMOCORE_GTK4)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK4 REQUIRED gtk4 glib-2.0)
  # ⚠️ GTK4SRC doit primer sur include/ : les fichiers cœur de gtk4sermo et le
  # lexer/parser GÉNÉRÉ (dans le build dir) doivent voir LE MÊME automaton.h.
  # L'énum WIDGET_* de gtk4sermo diffère de l'étalon (bloc 0xB4-0xC3 réordonné).
  # Si le parser voyait include/automaton.h (étalon) et automaton.c voyait
  # GTK4SRC/automaton.h (gtk4), password→0xBC serait créé comme spinner, etc.
  # BEFORE place GTK4SRC en tête du chemin d'inclusion ; include/ reste en repli
  # pour config.h / sermo_backend.h absents de GTK4SRC.
  target_include_directories(sermocore BEFORE PUBLIC ${GTK4SRC})
  target_include_directories(sermocore PUBLIC ${GTK4_INCLUDE_DIRS})
  target_compile_options(sermocore PRIVATE
    -Wno-deprecated-declarations   # appels GTK 4 dépréciés : chantier suivi à part
    -include ${GTK4SRC}/gtk4-compat.h -DSERMO_GLADE=1 ${GTK4_CFLAGS_OTHER})
  sermocore_vte(vte-2.91-gtk4 libvte-2.91-gtk4-dev)
  message(STATUS "sermocore : variante GTK4 (fichiers de gtk4sermo)")
elseif(SERMOCORE_GLIB)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  target_include_directories(sermocore PUBLIC ${GTK3_INCLUDE_DIRS})
  # SERMO_GLADE : --glade-xml charge un fichier GtkBuilder (glade_support.c du
  # backend gtk3). Même drapeau dans la variante GTK4 ; la neutre refuse l'option.
  target_compile_options(sermocore PRIVATE ${GTK3_CFLAGS_OTHER} -DSERMO_GLADE=1)
  sermocore_vte(vte-2.91 libvte-2.91-dev)
  message(STATUS "sermocore : variante GLIB/GTK3 (famille gtk3)")
else()
  target_include_directories(sermocore BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/_shim)
  target_compile_options(sermocore PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/include/sermocore-shim.h)
  message(STATUS "sermocore : variante NEUTRE (shim, sans toolkit)")
endif()

# ─── Install + pkg-config (1.3) ──────────────────────────────────────────────
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sermocore.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/sermocore.pc @ONLY)
install(TARGETS sermocore ARCHIVE DESTINATION ${SERMOCORE_LIBDIR})
# En-têtes : ceux du cœur unifié, plus (variante GTK4) ceux de gtk4sermo pour que
# le backend gtk4 dispose de gtk4-compat.h, gtk3d.h, etc.
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include/sermo
        FILES_MATCHING PATTERN "*.h" PATTERN "_shim" EXCLUDE)
if(SERMOCORE_GTK4)
  # ⚠️ PIÈGE MESURÉ (2026-09-13) : file(INSTALL) saute un fichier dont la SOURCE
  # n'est pas plus RÉCENTE que la destination — il compare les horodatages, pas
  # le contenu. include/ vient d'être installé juste au-dessus avec les mêmes
  # noms de fichiers ; un automaton.h de src-gtk4/ daté d'avant est donc
  # SILENCIEUSEMENT ignoré, et le backend gtk4 compile contre l'énum de
  # l'étalon. C'est le bug des DEUX ÉNUMS (corrigé le 2026-09-09, réapparu le
  # 2026-09-13 : filechooser rendait une date, password rendait "1"). On efface
  # donc d'abord, nommément, les en-têtes que src-gtk4/ va fournir.
  install(CODE "
    file(GLOB _gtk4_hdrs \"${GTK4SRC}/*.h\")
    foreach(_h IN LISTS _gtk4_hdrs)
      get_filename_component(_n \"\${_h}\" NAME)
      file(REMOVE \"\${CMAKE_INSTALL_PREFIX}/include/sermo/\${_n}\")
    endforeach()")
  install(DIRECTORY ${GTK4SRC}/ DESTINATION include/sermo FILES_MATCHING PATTERN "*.h")
endif()
# Les en-têtes générés par bison et flex (gtkdialog_parser.h, gtkdialog_lexer.h)
# ne s'installent pas : aucun backend ne les inclut, et ils portent le chemin
# absolu du dossier de construction (directives #line, garde d'inclusion bison),
# qui finissait dans le paquet sermo-core-dev.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sermocore.pc DESTINATION ${SERMOCORE_LIBDIR}/pkgconfig)
