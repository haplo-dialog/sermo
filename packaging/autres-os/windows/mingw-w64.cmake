# SPDX-License-Identifier: GPL-2.0-or-later
# Chaîne de compilation croisée Debian → Windows 64 bits (mingw-w64).
# Usage : cmake -DCMAKE_TOOLCHAIN_FILE=packaging/autres-os/windows/mingw-w64.cmake …
# Paquet Debian : gcc-mingw-w64-ucrt64.
#
# UCRT, le CRT de Windows 10 et 11, et non l'ancien msvcrt.dll : msvcrt ne
# connaît pas de locale UTF-8, et ncurses y relisait le texte UTF-8 comme du
# Windows-1252 (« à » affiché « Ã  », mesuré le 2026-10-11 sous Wine).
# -DSERMO_MINGW_TRIPLET=x86_64-w64-mingw32 (paquet gcc-mingw-w64-x86-64)
# reconstruit sur msvcrt, pour comparer.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
if(NOT DEFINED SERMO_MINGW_TRIPLET)
  set(SERMO_MINGW_TRIPLET x86_64-w64-mingw32ucrt)
endif()
set(MINGW_TRIPLET ${SERMO_MINGW_TRIPLET})
set(CMAKE_C_COMPILER   ${MINGW_TRIPLET}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_TRIPLET}-g++)
set(CMAKE_RC_COMPILER  ${MINGW_TRIPLET}-windres)
# Bibliothèques et en-têtes : ceux de la cible seulement ; les programmes
# (flex, bison, pkg-config) : ceux du poste qui construit.
set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_TRIPLET} ${SERMO_MINGW_ROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
# printf de mingw-w64 (C99) plutôt que celui du CRT Microsoft : ce dernier
# arrondit 2,5 en « 3 » avec %.0f, la glibc en « 2 » (mesuré le 2026-10-11).
set(CMAKE_C_FLAGS_INIT "-D__USE_MINGW_ANSI_STDIO=1")
