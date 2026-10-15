# SPDX-License-Identifier: GPL-2.0-or-later
# sermo-source — programmes d'aide de <input modbus> et <input opcua>
# (voir libsermocore/include/sermo_source.h). Chacun est construit si sa
# bibliothèque est présente : libmodbus-dev, libopen62541-1.4-dev.
# Installés dans <prefix>/libexec/sermo, le dossier que lit le cœur.
cmake_minimum_required(VERSION 3.16)
project(sermo-source C)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MODBUS IMPORTED_TARGET libmodbus)
pkg_check_modules(OPEN62541 IMPORTED_TARGET open62541)

# ON (CI, paquets) : une bibliothèque absente arrête la construction au lieu
# de sauter son programme d'aide.
option(SERMO_SOURCE_EXIGER "libmodbus et open62541 obligatoires" OFF)

set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -Wformat=2 -Wformat-security)

set(construits "")
if(MODBUS_FOUND)
  add_executable(sermo-source-modbus sermo-source-modbus.c commun.c)
  target_link_libraries(sermo-source-modbus PRIVATE PkgConfig::MODBUS m)
  list(APPEND construits sermo-source-modbus)
elseif(SERMO_SOURCE_EXIGER)
  message(FATAL_ERROR "libmodbus absente (paquet libmodbus-dev)")
else()
  message(WARNING "libmodbus absente : sermo-source-modbus n'est PAS construit (paquet libmodbus-dev)")
endif()
if(OPEN62541_FOUND)
  add_executable(sermo-source-opcua sermo-source-opcua.c commun.c)
  target_link_libraries(sermo-source-opcua PRIVATE PkgConfig::OPEN62541)
  list(APPEND construits sermo-source-opcua)
elseif(SERMO_SOURCE_EXIGER)
  message(FATAL_ERROR "open62541 absente (paquet libopen62541-1.4-dev)")
else()
  message(WARNING "open62541 absente : sermo-source-opcua n'est PAS construit (paquet libopen62541-1.4-dev)")
endif()

if(construits)
  install(TARGETS ${construits} RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/sermo)
endif()
