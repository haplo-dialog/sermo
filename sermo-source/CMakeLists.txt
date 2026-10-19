# SPDX-License-Identifier: GPL-2.0-or-later
# sermo-source — programmes d'aide de <input modbus>, <input opcua>, <input mqtt>,
# <input mavlink>, et d'écriture de <action mqtt> et <action mavlink>
# (voir libsermocore/include/sermo_source.h). Chacun est construit si sa
# bibliothèque est présente : libmodbus-dev, libopen62541-1.4-dev,
# libmosquitto-dev.
# Installés dans <prefix>/libexec/sermo, le dossier que lit le cœur.
cmake_minimum_required(VERSION 3.16)
project(sermo-source C)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MODBUS IMPORTED_TARGET libmodbus)
pkg_check_modules(OPEN62541 IMPORTED_TARGET open62541)
pkg_check_modules(MOSQUITTO IMPORTED_TARGET libmosquitto)

# ON (CI, paquets) : une bibliothèque absente arrête la construction au lieu
# de sauter son programme d'aide.
option(SERMO_SOURCE_EXIGER "libmodbus, open62541 et libmosquitto obligatoires" OFF)

# Même durcissement que les ports et sermoia-mcp. Absent jusqu'à la 3.0.0
# comprise : garde_durcissement.sh, joué pour la première fois sur les
# programmes d'aide le 2026-10-16, trouvait CET IBT, CET SHSTK et FORTIFY
# absents des trois (le banc ne visait que les ports). FORTIFY n'agit qu'avec
# l'optimisation : Release par défaut.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -Wformat=2 -Wformat-security)
# Protection du flot de contrôle selon le processeur : CET sur x86, BTI/PAC sur
# ARM64 (écrite en dur, -fcf-protection arrête une construction ARM64).
include(CheckCCompilerFlag)
check_c_compiler_flag(-fcf-protection=full SERMO_A_CF_PROTECTION)
check_c_compiler_flag(-mbranch-protection=standard SERMO_A_BRANCH_PROTECTION)
if(SERMO_A_CF_PROTECTION)
  set(SERMO_FLOT_CONTROLE -fcf-protection=full)
  set(SERMO_CET_LIEN -Wl,-z,ibt -Wl,-z,shstk)
elseif(SERMO_A_BRANCH_PROTECTION)
  set(SERMO_FLOT_CONTROLE -mbranch-protection=standard)
endif()
function(durcir cible)
  target_compile_options(${cible} PRIVATE -D_FORTIFY_SOURCE=3 -fstack-protector-strong
    -fstack-clash-protection ${SERMO_FLOT_CONTROLE})
  target_link_options(${cible} PRIVATE -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
    ${SERMO_CET_LIEN} ${SERMO_FLOT_CONTROLE} -fstack-protector-strong -fstack-clash-protection)
endfunction()

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
if(MOSQUITTO_FOUND)
  add_executable(sermo-source-mqtt sermo-source-mqtt.c mqtt-commun.c commun.c)
  target_link_libraries(sermo-source-mqtt PRIVATE PkgConfig::MOSQUITTO)
  list(APPEND construits sermo-source-mqtt)
  # L'ÉCRITURE domotique, dans son propre paquet (sermo-cible-mqtt).
  add_executable(sermo-cible-mqtt sermo-cible-mqtt.c mqtt-commun.c commun.c)
  target_link_libraries(sermo-cible-mqtt PRIVATE PkgConfig::MOSQUITTO m)
  list(APPEND construits sermo-cible-mqtt)
elseif(SERMO_SOURCE_EXIGER)
  message(FATAL_ERROR "libmosquitto absente (paquet libmosquitto-dev)")
else()
  message(WARNING "libmosquitto absente : sermo-source-mqtt n'est PAS construit (paquet libmosquitto-dev)")
endif()

# <input media> / <action media> : lecteur son/vidéo. mpv est un
# PROGRAMME externe (Recommends), pas une bibliothèque : toujours construit.
add_executable(sermo-source-media sermo-source-media.c media-commun.c commun.c)
add_executable(sermo-cible-media sermo-cible-media.c media-commun.c commun.c)
list(APPEND construits sermo-source-media sermo-cible-media)

# <input mavlink> : en-têtes C de MAVLink embarqués (tiers/mavlink, MIT ; voir
# son LISEZMOI). Aucune bibliothèque à trouver : toujours construit. SYSTEM :
# les avertissements des en-têtes engendrés ne sont pas les nôtres.
add_executable(sermo-source-mavlink sermo-source-mavlink.c mavlink-commun.c commun.c)
target_include_directories(sermo-source-mavlink SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tiers/mavlink)
list(APPEND construits sermo-source-mavlink)
# L'ÉCRITURE drone (réglages au sol, ordres de sécurité), dans son propre
# paquet (sermo-cible-mavlink).
add_executable(sermo-cible-mavlink sermo-cible-mavlink.c mavlink-commun.c mission-wpl.c commun.c)
target_include_directories(sermo-cible-mavlink SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tiers/mavlink)
target_link_libraries(sermo-cible-mavlink PRIVATE m)
list(APPEND construits sermo-cible-mavlink)

foreach(c ${construits})
  durcir(${c})
endforeach()
if(construits)
  install(TARGETS ${construits} RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/sermo)
endif()
