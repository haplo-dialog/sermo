# SPDX-License-Identifier: GPL-2.0-or-later
# sermoia-mcp — serveur MCP pour sermo (boucle IA apprendre→générer→vérifier→
# voir→corriger).
cmake_minimum_required(VERSION 3.16)
# Version : une seule source, le fichier VERSION à la racine du dépôt.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)
project(sermoia-mcp VERSION ${SERMO_VERSION} LANGUAGES C)

# Le lien se fait en -pie : le code doit être compilé en -fPIE. Debian le fait par
# défaut, pas toutes les distributions (Slackware 15 : « relocation R_X86_64_32 …
# recompile with -fPIE », mesuré le 2026-10-10).
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED glib-2.0 gio-2.0 json-glib-1.0)

add_executable(sermoia-mcp src/sermoia-mcp.c)
target_include_directories(sermoia-mcp PRIVATE ${DEPS_INCLUDE_DIRS})
target_compile_definitions(sermoia-mcp PRIVATE SERMOIA_MCP_VERSION="${PROJECT_VERSION}")
target_compile_options(sermoia-mcp PRIVATE
  -Wall -Wextra -std=gnu11 ${DEPS_CFLAGS_OTHER}
  -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection -fcf-protection=full)
target_link_options(sermoia-mcp PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,ibt -Wl,-z,shstk
  -fcf-protection=full -fstack-protector-strong -fstack-clash-protection)
target_link_libraries(sermoia-mcp PRIVATE ${DEPS_LIBRARIES})
target_link_directories(sermoia-mcp PRIVATE ${DEPS_LIBRARY_DIRS})
