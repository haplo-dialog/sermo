# sermo-backend-html — backend d'EXPORT HTML STATIQUE, lié sur libsermocore
# (variante NEUTRE). Le CŒUR (parse/automaton/variables/actions/gtkdialog.c+
# main/lexer/parser) vit dans libsermocore ; ce backend ne garde que ses
# widgets (modèle arbre-de-WidgetNode partagé avec les ports sdl3/ncurses,
# copiés depuis ncurses car déjà 100% agnostiques du moteur de rendu), le
# bridge d'attributs (tag_set_property.c), le hook d'init + l'ABI neutre
# (sermo_backend_html.c) et l'exporteur (render_html.c). Étape de faisabilité
# (ADR dédié) : prouve que l'arbre neutre s'exporte en HTML, pas encore
# l'interactif en ligne.
cmake_minimum_required(VERSION 3.16)
# Version : une seule source, le fichier VERSION à la racine du dépôt.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)

project(htmlsermo VERSION ${SERMO_VERSION} LANGUAGES C)

# Le lien se fait en -pie : le code doit être compilé en -fPIE. Debian le fait par
# défaut, pas toutes les distributions (Slackware 15 : « relocation R_X86_64_32 …
# recompile with -fPIE », mesuré le 2026-10-10).
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# config.h est ENGENDRÉ ici depuis config.h.in (version = fichier VERSION).
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

# ─── Sources backend ───────────────────────────────────────────────────────
# widget*.c capture widget_*.c + widgets.c + widget_node.c + widget_stubs.c.
file(GLOB WIDGET_SRC "src/widget*.c")

# neutral_glist.c : fournit g_list_append EXTERNE au cœur (le shim
# html-compat.h ne donne qu'un static inline, aucun symbole exporté). Compilé
# SANS le force-include du shim, sinon conflit de définition (cf. ncurses/sdl3).
add_library(sermo_neutral_glist OBJECT src/neutral_glist.c)
target_compile_options(sermo_neutral_glist PRIVATE -w -fPIE)

add_executable(htmlsermo
  ${WIDGET_SRC}
  src/tag_set_property.c
  src/printing.c
  src/sermo_backend_html.c
  src/render_html.c
  src/serve_http.c          # micro-serveur local (SERMO_HTML_MODE=serve)
  $<TARGET_OBJECTS:sermo_neutral_glist>
)

target_include_directories(htmlsermo BEFORE PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}            # config.h engendré
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract # contrat sermo_be_*
  ${CMAKE_CURRENT_SOURCE_DIR}/src        # en-têtes locaux (mêmes que le cœur neutre)
  ${SERMOCORE_INCLUDE_DIRS})

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
# Les objets à part sont liés dans le même binaire : un seul objet sans la
# protection du flot de contrôle et le lieur la retire du binaire entier
# (mesuré sur ARM64 avec -z bti-report). Même durcissement que le binaire.
target_compile_options(sermo_neutral_glist PRIVATE
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection ${SERMO_FLOT_CONTROLE})
target_compile_options(htmlsermo PRIVATE
  -w -std=gnu11
  # html-compat.h : shim GLib auto-suffisant (GList, g_*), pas de glib réelle
  -include ${CMAKE_CURRENT_SOURCE_DIR}/src/html-compat.h
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection ${SERMO_FLOT_CONTROLE})

# Durcissement au lien : PIE/RELRO/BIND_NOW/NX + CET (IBT/SHSTK forcés, cf. qt6/ncurses).
# Marquage CET (-z ibt/shstk) : x86 seulement, comme -fcf-protection.
if(SERMO_A_CF_PROTECTION)
  set(SERMO_CET_LIEN -Wl,-z,ibt -Wl,-z,shstk)
endif()
target_link_options(htmlsermo PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
  ${SERMO_CET_LIEN}
  ${SERMO_FLOT_CONTROLE} -fstack-protector-strong -fstack-clash-protection)

target_link_libraries(htmlsermo PRIVATE
  ${SERMOCORE_LINK_LIBRARIES})     # cœur via pkg-config (variante neutre, lien statique)

# ─── Installation ─────────────────────────────────────────────────────────────
set(PACKAGE_VERSION "${PROJECT_VERSION}")
install(TARGETS htmlsermo DESTINATION bin)

# Ce port n'a pas de page propre : il reçoit la page générique du dépôt, sous
# son propre nom — comme ncurses.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../doc/sermo.1.in"
  "${CMAKE_CURRENT_BINARY_DIR}/htmlsermo.1"
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/htmlsermo.1 DESTINATION share/man/man1)

# ─── Le port se nomme lui-même ────────────────────────────────────────────────
# contract/sermo_port_id.c remplit les deux symboles faibles que le cœur
# déclare : « --version » dit « htmlsermo », pas « sermocore ».
target_sources(htmlsermo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract/sermo_port_id.c)
target_compile_definitions(htmlsermo PRIVATE
  SERMO_PORT_NAME="${PROJECT_NAME}"
  SERMO_PORT_VERSION="${PROJECT_VERSION}")
