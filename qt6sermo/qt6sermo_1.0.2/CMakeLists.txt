cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_policy(SET CMP0115 NEW)  # Explicit source file extensions required
project(qt6sermo VERSION 1.0.2 LANGUAGES C CXX)

# config.h est ENGENDRÉ ici, pas versionné : une copie écrite à la main était
# ignorée par git, donc absente de tout clone propre — le port ne compilait
# pas ailleurs que sur le poste d'origine. La version suit désormais
# project(... VERSION ...) toute seule.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
  @ONLY)
# Default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


# ─── Politique CMake ────────────────────────────────────────────────────────
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)   # MOC automatique pour les classes QObject
set(CMAKE_AUTORCC ON)   # Ressources Qt (.qrc)
set(CMAKE_AUTOUIC OFF)  # Pas de .ui files — layout via XML gtk3d

# ─── Qt6 ─────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
message(STATUS "Qt6 version: ${Qt6_VERSION}")
if(Qt6_VERSION VERSION_LESS "6.2.0")
  message(FATAL_ERROR "Qt6 >= 6.2.0 required")
endif()

# ─── QTermWidget (terminal VTE-équivalent) ────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(QTERMWIDGET qtermwidget6)
if(QTERMWIDGET_FOUND)
  message(STATUS "QTermWidget found — <terminal> widget available")
  add_compile_definitions(HAVE_QTERMWIDGET=1)
else()
  message(WARNING "QTermWidget not found — <terminal> widget will be a stub")
  add_compile_definitions(HAVE_QTERMWIDGET=0)
endif()

# ─── Flex / Bison (parser XML) ───────────────────────────────────────────────
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# Le préfixe « gtkdialog_ » doit être imposé sur flex/bison : les sources .l/.y
# référencent gtkdialog_lval / gtkdialog_parse (et non yylval / yyparse).
# Les ports autotools le font via AM_LFLAGS=-P… / AM_YFLAGS=-p… ; on réplique ici.
FLEX_TARGET(GtkdialogLexer
  src/gtkdialog_lexer.l
  ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_lexer.c
  COMPILE_FLAGS "-P gtkdialog_"
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_lexer.h
)
BISON_TARGET(GtkdialogParser
  src/gtkdialog_parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_parser.c
  COMPILE_FLAGS "-p gtkdialog_"
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/gtkdialog_parser.h
)
ADD_FLEX_BISON_DEPENDENCY(GtkdialogLexer GtkdialogParser)

# ─── Sources ─────────────────────────────────────────────────────────────────
set(CORE_SOURCES
  src/gtkdialog.c
  src/safe_exec.c
  src/variables.c
  src/stack.c
  src/automaton.c
  src/attributes.c
  src/signals.c
  src/stringman.c
  src/actions.c
  src/tag_attributes.c
  ${FLEX_GtkdialogLexer_OUTPUTS}
  ${BISON_GtkdialogParser_OUTPUTS}
)

set(WIDGET_SOURCES
  src/qt6-compat.cpp
  src/widget_stubs.cpp
  src/widget_image.cpp
  src/widget_pulse.cpp
  src/widget_linkbutton.cpp
  src/widget_eventbox.cpp
  src/widget_filechooser.cpp
  src/widgets.cpp
  src/widget_window.cpp
  src/widget_button.cpp
  src/widget_checkbox.cpp
  src/widget_colorbutton.cpp
  src/widget_combobox.cpp
  src/widget_comboboxtext.cpp
  src/widget_edit.cpp
  src/widget_entry.cpp
  src/widget_expander.cpp
  src/widget_fontbutton.cpp
  src/widget_frame.cpp
  src/widget_hbox.cpp
  src/widget_hscale.cpp
  src/widget_hseparator.cpp
  src/widget_list.cpp
  src/widget_menubar.cpp
  src/widget_menuitem.cpp
  src/widget_notebook.cpp
  src/widget_pixmap.cpp
  src/widget_progressbar.cpp
  src/widget_radiobutton.cpp
  src/widget_spinbutton.cpp
  src/widget_statusbar.cpp
  src/widget_table.cpp
  src/widget_terminal.cpp
  src/widget_text.cpp
  src/widget_timer.cpp
  src/widget_togglebutton.cpp
  src/widget_tree.cpp
  src/widget_vbox.cpp
  src/widget_vscale.cpp
  src/widget_vseparator.cpp
  # Nouveaux widgets v2
  src/widget_switch.cpp
  src/widget_password.cpp
  src/widget_searchentry.cpp
  src/widget_calendar.cpp
  src/widget_infobar.cpp
  src/widget_levelbar.cpp
  src/widget_spinner.cpp
  src/widget_aspectframe.cpp
  src/widget_drawingarea.cpp
)

# ─── Binaire ─────────────────────────────────────────────────────────────────
# data/qt6sermo.qrc embarque l'icône dans le binaire : sans elle, un qt6sermo
# non installé ne posait aucun _NET_WM_ICON et le gestionnaire de fenêtres
# affichait une icône générique (voir le commentaire du .qrc).
add_executable(qt6sermo ${CORE_SOURCES} ${WIDGET_SOURCES} data/qt6sermo.qrc)

target_include_directories(qt6sermo PRIVATE
  src/
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QTERMWIDGET_INCLUDE_DIRS}
)

target_compile_options(qt6sermo PRIVATE
  -Wall -Wextra -Wno-unused-parameter
  -Wno-deprecated-declarations
  -Wno-unused-variable
  $<$<COMPILE_LANGUAGE:C>:-Wno-missing-prototypes>
  -Wshadow
  -Wnull-dereference
  -Wimplicit-fallthrough=3
  -Wformat -Wformat-overflow=2 -Werror=format-security
  -D_FORTIFY_SOURCE=3
  -D_POSIX_C_SOURCE=200809L
  -fstack-protector-strong
  -fstack-clash-protection
  -fcf-protection=full
  -fPIE
  # Note: CMAKE_POSITION_INDEPENDENT_CODE est géré via -fPIE/-pie explicites
  # Force-include le header de compatibilité dans chaque unité C
  $<$<COMPILE_LANGUAGE:C>:-include ${CMAKE_CURRENT_SOURCE_DIR}/src/qt6-compat.h>
)

target_link_options(qt6sermo PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now
  -Wl,-z,noexecstack
  # Ces deux -z FORCENT la note ELF « x86 feature: IBT, SHSTK » dans le binaire.
  # ⚠️ Indispensable : sur Debian testing (drift du 2026-09), Scrt1.o/crti.o de
  # la glibc ne portent plus la note, et la fusion du lieur est un ET — chaque
  # binaire lié la perdait, -fcf-protection=full ou pas (mesuré au readelf le
  # 2026-09-03 : les .o l'avaient tous, le binaire non ; un main() trivial la
  # perdait aussi). Les ports GTK posent ces mêmes drapeaux (configure.ac:99).
  -Wl,-z,ibt -Wl,-z,shstk
  # Répétés au lien pour la recompilation LTO :
  -fcf-protection=full -fstack-protector-strong -fstack-clash-protection
)

target_link_libraries(qt6sermo PRIVATE
  Qt6::Core
  Qt6::Widgets
  ${QTERMWIDGET_LIBRARIES}
)


# ─── LTO (Link-Time Optimization) en Release ─────────────────────────────
# NB : la perte de la note CET constatée le 2026-09-03 ne venait PAS de la LTO
# mais des crt de la glibc testing (voir -Wl,-z,ibt/shstk ci-dessus) — un lien
# trivial sans LTO la perdait aussi. La LTO reste donc active.
include(CheckIPOSupported)
check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_err)
if(_ipo_ok AND CMAKE_BUILD_TYPE STREQUAL "Release")
  set_property(TARGET qt6sermo PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  message(STATUS "LTO: enabled")
else()
  message(STATUS "LTO: disabled (${_ipo_err})")
endif()

# ─── Installation ────────────────────────────────────────────────────────────
install(TARGETS qt6sermo DESTINATION bin)
install(FILES src/qt6sermo.1
  DESTINATION share/man/man1)
install(FILES data/icons/hicolor/32x32/apps/qt6sermo.png
  DESTINATION share/icons/hicolor/32x32/apps
  OPTIONAL
)
# Lanceur .desktop (masqué — outil en ligne de commande) et métadonnées AppStream
install(FILES data/qt6sermo.desktop
  DESTINATION share/applications)
install(FILES data/fr.haplo_dialog.qt6sermo.metainfo.xml
  DESTINATION share/metainfo)

# ─── Analyse statique (optionnel) ──────────────────────────────────────────
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
  add_custom_target(cppcheck
    COMMAND ${CPPCHECK}
      --enable=all --suppress=missingInclude --suppress=unusedFunction
      --std=c11 --std=c++17
      -I ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/src
    COMMENT "Running cppcheck static analysis"
  )
endif()

# ─── Tests CTest ──────────────────────────────────────────────────────────────
enable_testing()

# GLib — nécessaire pour safe_exec.c dans les tests
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(GLIB QUIET glib-2.0)
endif()

if(GLIB_FOUND)
  # test_safe_exec : compile safe_exec.c standalone + test C
  add_executable(test_safe_exec
    tests/test_safe_exec.c
    src/safe_exec.c
  )
  target_include_directories(test_safe_exec PRIVATE
    src/
    ${GLIB_INCLUDE_DIRS}
  )
  target_compile_options(test_safe_exec PRIVATE
    ${GLIB_CFLAGS_OTHER}
    -Wall -Wextra -Wno-unused-parameter
  )
  target_link_libraries(test_safe_exec PRIVATE ${GLIB_LIBRARIES})
  add_test(NAME safe_exec COMMAND test_safe_exec)

  # test_compat : compile qt6-compat.h uniquement, test C++
  add_executable(test_compat
    tests/test_compat.cpp
  )
  target_include_directories(test_compat PRIVATE
    src/
    ${GLIB_INCLUDE_DIRS}
  )
  target_compile_options(test_compat PRIVATE
    ${GLIB_CFLAGS_OTHER}
    -Wall -Wextra -Wno-unused-parameter
  )
  target_compile_definitions(test_compat PRIVATE HAVE_QTERMWIDGET=0)
  target_link_libraries(test_compat PRIVATE ${GLIB_LIBRARIES})
  add_test(NAME compat COMMAND test_compat)

  message(STATUS "CTest: tests enabled (GLib found)")
else()
  message(WARNING "CTest: GLib not found — tests skipped (install libglib2.0-dev)")
endif()

# ─── Résumé de configuration ─────────────────────────────────────────────────
message(STATUS "")
message(STATUS "qt6sermo 1.0.0 configuration:")
message(STATUS "  Qt6 version    : ${Qt6_VERSION}")
message(STATUS "  QTermWidget    : ${QTERMWIDGET_FOUND}")
message(STATUS "  Install prefix : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
