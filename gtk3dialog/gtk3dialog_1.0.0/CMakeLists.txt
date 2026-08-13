# CMakeLists.txt — gtk3dialog 1.0.0 (haplo-dialog)
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Build CMake du port de référence. Cohabite avec autotools : c'est le
# patron de convergence destiné à être décliné sur les autres ports.
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build -j
#   ctest --test-dir build
#
cmake_minimum_required(VERSION 3.16)
project(gtk3dialog VERSION 1.0.0 LANGUAGES C)

option(WITH_VTE "Activer le widget terminal (VTE 2.91)" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ── Dépendances ───────────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0>=3.22.0)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0>=2.56)
pkg_check_modules(GTHREAD IMPORTED_TARGET gthread-2.0)

set(HAVE_VTE 0)
if(WITH_VTE)
  pkg_check_modules(VTE IMPORTED_TARGET vte-2.91)
  if(VTE_FOUND)
    set(HAVE_VTE 1)
  else()
    message(WARNING "VTE 2.91 introuvable — widget terminal désactivé")
  endif()
endif()

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# ── config.h généré ─────────────────────────────────────────────────────────
configure_file(config.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

# ── Parser / lexer (préfixe gtkdialog_) ─────────────────────────────────────
bison_target(GtkdialogParser
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gtkdialog_parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_parser.c
  COMPILE_FLAGS "-d -p gtkdialog_"
  DEFINES_FILE  ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_parser.h)
flex_target(GtkdialogLexer
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gtkdialog_lexer.l
  ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_lexer.c
  COMPILE_FLAGS "-P gtkdialog_")
add_flex_bison_dependency(GtkdialogLexer GtkdialogParser)

# ── Sources (toutes les .c écrites à la main du dossier src/) ────────────────
file(GLOB GTK3DIALOG_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_executable(gtk3dialog
  ${GTK3DIALOG_SOURCES}
  ${BISON_GtkdialogParser_OUTPUTS}
  ${FLEX_GtkdialogLexer_OUTPUTS})

target_include_directories(gtk3dialog PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR})           # config.h + parser/lexer générés

target_compile_definitions(gtk3dialog PRIVATE HAVE_CONFIG_H HAVE_VTE=${HAVE_VTE})

# ── Durcissement (aligné sur la CI) ─────────────────────────────────────────
target_compile_options(gtk3dialog PRIVATE
  -Wall -Wextra
  -Werror=format-security
  -D_FORTIFY_SOURCE=3
  -fstack-protector-strong
  -fstack-clash-protection
  -Wno-deprecated-declarations -Wno-unused-variable -Wno-missing-prototypes)
target_link_options(gtk3dialog PRIVATE -pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack)

target_link_libraries(gtk3dialog PRIVATE PkgConfig::GTK3 PkgConfig::GLIB)
if(GTHREAD_FOUND)
  target_link_libraries(gtk3dialog PRIVATE PkgConfig::GTHREAD)
endif()
if(HAVE_VTE)
  target_link_libraries(gtk3dialog PRIVATE PkgConfig::VTE)
endif()

install(TARGETS gtk3dialog RUNTIME DESTINATION bin)

# ── Tests (CTest) ───────────────────────────────────────────────────────────
enable_testing()
pkg_check_modules(CHECK IMPORTED_TARGET check>=0.9.6)
if(CHECK_FOUND)
  add_executable(test_safe_exec
    tests/test_safe_exec.c
    src/safe_exec.c)
  target_include_directories(test_safe_exec PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
  target_compile_definitions(test_safe_exec PRIVATE HAVE_CONFIG_H)
  target_link_libraries(test_safe_exec PRIVATE PkgConfig::CHECK PkgConfig::GLIB)
  add_test(NAME safe_exec COMMAND test_safe_exec)
else()
  message(STATUS "libcheck introuvable — tests unitaires C désactivés")
endif()
