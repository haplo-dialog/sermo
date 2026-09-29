# SPDX-License-Identifier: GPL-2.0-or-later
# sermo-backend-sdl3 — backend de rendu SDL3 + Dear ImGui (immediate-mode),
# lie sur libsermocore (variante NEUTRE).
# Le CŒUR (parse/automaton/variables/actions/gtkdialog.c+main/lexer/parser) vit
# dans libsermocore ; ce backend ne garde que ses widgets + le bridge d'attributs
# (tag_set_property.c) + le hook d'init toolkit et l'ABI neutre (sermo_backend_sdl3sermo.c).
cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0115 NEW)
# Version : une seule source, le fichier VERSION à la racine du dépôt. Rien
# n'est à resynchroniser ici ; tests/garde_version.sh vérifie que tout le dépôt
# s'y accorde.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)

project(sdl3sermo VERSION ${SERMO_VERSION} LANGUAGES C CXX)

# config.h est ENGENDRÉ ici, pas versionné : la copie écrite à la main portait
# la version en dur, avec la consigne de la « garder en accord » avec
# CMakeLists.txt. Elle suit désormais project(... VERSION ...), donc VERSION.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
  @ONLY)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Cœur partage via pkg-config (variante NEUTRE) ────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(SERMOCORE REQUIRED sermocore)

# ─── SDL3 + OpenGL (backend de rendu ImGui) ───────────────────────────────
find_package(SDL3 REQUIRED CONFIG)
find_package(OpenGL REQUIRED)
find_package(X11)

# ─── libpng (ecriture PNG pour le rendu offscreen --render-png) ───────────
pkg_check_modules(LIBPNG REQUIRED IMPORTED_TARGET libpng)

# ─── Dear ImGui (vendored dans src/imgui/) ────────────────────────────────
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui)
add_library(imgui STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3 OpenGL::GL)
target_compile_options(imgui PRIVATE -w)

# ─── Sources backend ──────────────────────────────────────────────────────
# widget*.c capture widget_*.c ET widgets.c/widget_node.c/widget_stubs.c
file(GLOB WIDGET_SRC "src/widget*.c")
set(BACKEND_C
  ${WIDGET_SRC}
  src/tag_set_property.c            # bridge attributs (sorti du coeur)
  src/sermo_backend_sdl3sermo.c     # hook init toolkit + ABI neutre qt6_*
  src/printing.c
  src/glade_support.c
)
set(BACKEND_CXX
  src/render.cpp                    # boucle SDL3/OpenGL/ImGui + dispatch arbre
)

# image_load.c : voit la VRAIE GLib (gdk-pixbuf), JAMAIS sdl3-compat.h — objet a part.
pkg_check_modules(GDKPIXBUF REQUIRED IMPORTED_TARGET gdk-pixbuf-2.0)
add_library(sermo_image_load OBJECT src/image_load.c)
target_include_directories(sermo_image_load PRIVATE src/)
target_link_libraries(sermo_image_load PRIVATE PkgConfig::GDKPIXBUF)
target_compile_options(sermo_image_load PRIVATE -fPIE)

# neutral_glist.c : fournit g_list_append EXTERNE au coeur — compile SANS le
# force-include sdl3-compat.h (qui n'offre qu'un static inline). Objet a part.
add_library(sermo_neutral_glist OBJECT src/neutral_glist.c)
target_compile_options(sermo_neutral_glist PRIVATE -fPIE)

# ─── Binaire ──────────────────────────────────────────────────────────────
add_executable(sdl3sermo
  ${BACKEND_C} ${BACKEND_CXX}
  $<TARGET_OBJECTS:sermo_image_load>
  $<TARGET_OBJECTS:sermo_neutral_glist>
)

target_include_directories(sdl3sermo BEFORE PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}                  # config.h engendré
  src/
  ${SERMOCORE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract      # contrat sermo_be_* (MIT), rapatrie dans le depot
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

target_compile_options(sdl3sermo PRIVATE
  -D_FORTIFY_SOURCE=3
  -D_POSIX_C_SOURCE=200809L
  -fstack-protector-strong
  -fstack-clash-protection
  -fcf-protection=full
  -fPIE
  # Force-include du shim du port sur les unites C (comme le port d'origine) ;
  # render.cpp (C++) voit la vraie ImGui et n'en veut pas.
  $<$<COMPILE_LANGUAGE:C>:-include ${CMAKE_CURRENT_SOURCE_DIR}/src/sdl3-compat.h>
)

target_link_options(sdl3sermo PRIVATE
  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
  # CET IBT/SHSTK forcés : les crt de la glibc testing ne portent plus la note et
  # la fusion du lieur est un ET (cf. qt6). Vérifié par garde_durcissement.
  -Wl,-z,ibt -Wl,-z,shstk
  -fcf-protection=full -fstack-protector-strong -fstack-clash-protection)

target_link_libraries(sdl3sermo PRIVATE
  ${SERMOCORE_LINK_LIBRARIES}        # coeur partage via pkg-config (lien statique)
  imgui
  SDL3::SDL3
  OpenGL::GL
  PkgConfig::GDKPIXBUF
  PkgConfig::LIBPNG
  $<$<BOOL:${X11_FOUND}>:${X11_LIBRARIES}>
)
target_link_directories(sdl3sermo PRIVATE ${SERMOCORE_LIBRARY_DIRS})


# ─── Installation ─────────────────────────────────────────────────────────────
# Ces règles manquaient : seuls qt6 et le cœur en avaient. Qui construit depuis
# les sources n'avait aucun moyen d'installer le résultat — binaire, page de
# manuel et icônes étaient à recopier à la main, et COMPILE.md ne pouvait que
# désigner le binaire dans _build/. debian/rules pose ces mêmes fichiers de son
# côté : les deux chemins livrent désormais la même chose.
set(PACKAGE_VERSION "${PROJECT_VERSION}")
install(TARGETS sdl3sermo DESTINATION bin)

# Ce port n'a pas de page propre : il reçoit la page générique du dépôt, sous
# son propre nom — exactement ce que fait debian/rules.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../doc/sermo.1.in"
  "${CMAKE_CURRENT_BINARY_DIR}/sdl3sermo.1"
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl3sermo.1 DESTINATION share/man/man1)

# Icône d'application, aux huit tailles du thème hicolor. OPTIONAL : une taille
# absente ne doit pas faire échouer l'installation.
foreach(taille 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256)
  install(FILES data/icons/hicolor/${taille}/apps/sdl3sermo.png
    DESTINATION share/icons/hicolor/${taille}/apps
    OPTIONAL)
endforeach()


# ─── Le port se nomme lui-même ────────────────────────────────────────────────
# « --version » est imprimé par le cœur, compilé une fois : il ne connaissait
# que son propre nom, et les sept ports s'annonçaient « sermocore ».
# contract/sermo_port_id.c remplit les deux symboles faibles que le cœur
# déclare, avec le nom et la version que CMake connaît déjà. La POSITION des
# mots ne bouge pas : seul le premier change (des scripts lisent le 3e).
target_sources(sdl3sermo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../contract/sermo_port_id.c)
target_compile_definitions(sdl3sermo PRIVATE
  SERMO_PORT_NAME="${PROJECT_NAME}"
  SERMO_PORT_VERSION="${PROJECT_VERSION}")
