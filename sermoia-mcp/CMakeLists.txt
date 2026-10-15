# SPDX-License-Identifier: GPL-2.0-or-later
# sermoia-mcp — serveur MCP pour sermo (boucle IA apprendre→générer→vérifier→
# voir→corriger).
cmake_minimum_required(VERSION 3.16)
# Version : une seule source, le fichier VERSION à la racine du dépôt.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)
project(sermoia-mcp VERSION ${SERMO_VERSION} LANGUAGES C)

# FORTIFY_SOURCE n'agit qu'avec l'optimisation : sans type de build, CMake ne
# passe aucun -O et la protection promise par SECURITY.md reste lettre morte
# (garde_durcissement : FORTIFY absent, mesuré le 2026-10-12). Même règle que
# les ports. Vérifié par tests/garde_durcissement.sh.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Le lien se fait en -pie : le code doit être compilé en -fPIE. Debian le fait par
# défaut, pas toutes les distributions (Slackware 15 : « relocation R_X86_64_32 …
# recompile with -fPIE », mesuré le 2026-10-10).
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED glib-2.0 gio-2.0 json-glib-1.0)

add_executable(sermoia-mcp src/sermoia-mcp.c)
target_include_directories(sermoia-mcp PRIVATE ${DEPS_INCLUDE_DIRS})
include(GNUInstallDirs)
target_compile_definitions(sermoia-mcp PRIVATE SERMOIA_MCP_VERSION="${PROJECT_VERSION}"
  SERMOIA_EXAMPLES_DIR="${CMAKE_INSTALL_FULL_DATADIR}/sermoia-mcp/examples")
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
target_compile_options(sermoia-mcp PRIVATE
  -Wall -Wextra -std=gnu11 ${DEPS_CFLAGS_OTHER}
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection ${SERMO_FLOT_CONTROLE})
# Marquage CET (-z ibt/shstk) : x86 seulement, comme -fcf-protection.
if(SERMO_A_CF_PROTECTION)
  set(SERMO_CET_LIEN -Wl,-z,ibt -Wl,-z,shstk)
endif()
target_link_options(sermoia-mcp PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack ${SERMO_CET_LIEN}
  ${SERMO_FLOT_CONTROLE} -fstack-protector-strong -fstack-clash-protection)
target_link_libraries(sermoia-mcp PRIVATE ${DEPS_LIBRARIES})
target_link_directories(sermoia-mcp PRIVATE ${DEPS_LIBRARY_DIRS})

# Installation : le serveur et son catalogue d'exemples (paquet sermoia-mcp).
install(TARGETS sermoia-mcp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY examples/ DESTINATION ${CMAKE_INSTALL_DATADIR}/sermoia-mcp/examples)
