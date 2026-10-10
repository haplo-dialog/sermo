#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 S. Cage
# haplo-dialog <devel@haplo-dialog.fr>
#
# packaging/autres-os/construire-os.sh — la construction modulaire de sermo,
# écrite UNE fois pour les recettes hors Debian (Arch, RPM, Gentoo, Slackware).
# C'est la même suite de gestes que debian/rules : le cœur dans ses variantes,
# puis chaque backend contre la variante qui lui convient, puis l'installation
# par `cmake --install` — ce sont les CMakeLists de chaque port qui savent
# quels fichiers il livre (binaire, page de manuel, icônes, lanceur qt6).
#
# Usage :
#   construire-os.sh construire <SRC> <BUILD> <port>...
#       construit le cœur (les variantes dont les ports ont besoin) et les ports.
#   construire-os.sh installer  <SRC> <BUILD> <port> <DESTDIR> [PREFIXE]
#       installe UN port dans DESTDIR (PREFIXE : /usr par défaut).
#   construire-os.sh coeur-dev  <SRC> <BUILD> <DESTDIR> [PREFIXE] [LIBDIR]
#       installe le cœur neutre distribué (bibliothèque statique, en-têtes,
#       sermocore.pc) ; LIBDIR vaut « lib » par défaut (« lib64 » sous Fedora).
#   construire-os.sh manuel     <SRC> <BUILD> <DESTDIR> [PREFIXE]
#       construit et installe le manuel Texinfo (sermo-doc.info).
#
# Ports : gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl ncurses html
# Environnement : VERSION (défaut : le fichier VERSION), JOBS (défaut : nproc),
#   CMAKE_EXTRA (arguments ajoutés à chaque configuration, ex. -DCMAKE_BUILD_TYPE=None).
set -eu

cmd=${1:-}; [ $# -ge 1 ] && shift
SRC=${1:-}; BUILD=${2:-}
[ -n "$cmd" ] && [ -d "$SRC/libsermocore" ] && [ -n "$BUILD" ] || {
    sed -n '/^# Usage/,/^# Ports/p' "$0" | sed 's/^# \{0,1\}//' >&2; exit 2; }
SRC=$(CDPATH= cd -- "$SRC" && pwd)
mkdir -p "$BUILD"; BUILD=$(CDPATH= cd -- "$BUILD" && pwd)
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 2)}
VERSION=${VERSION:-$(tr -d '[:space:]' < "$SRC/VERSION")}
CMAKE_EXTRA=${CMAKE_EXTRA:-}

variante() {  # port -> variante du cœur
    case "$1" in
        gtk3) echo glib ;;
        gtk4) echo gtk4 ;;
        qt6|fltk1|efl1|sdl3|lvgl|ncurses|html) echo neutre ;;
        *) echo "port inconnu : $1" >&2; exit 2 ;;
    esac
}
options_variante() {
    case "$1" in
        glib) echo "-DSERMOCORE_GLIB=ON" ;;
        gtk4) echo "-DSERMOCORE_GTK4=ON" ;;
        *)    echo "" ;;
    esac
}
etage() { echo "$BUILD/etage/$1"; }

construire_coeur() {  # variante
    v=$1
    [ -f "$(etage "$v")/lib/pkgconfig/sermocore.pc" ] && return 0
    # shellcheck disable=SC2046
    cmake -S "$SRC/libsermocore" -B "$BUILD/coeur-$v" $(options_variante "$v") \
          -DCMAKE_INSTALL_PREFIX="$(etage "$v")" $CMAKE_EXTRA
    cmake --build "$BUILD/coeur-$v" -j"$JOBS"
    cmake --install "$BUILD/coeur-$v"
}

case "$cmd" in
construire)
    shift 2
    [ $# -gt 0 ] || { echo "aucun port nommé" >&2; exit 2; }
    for p in "$@"; do
        v=$(variante "$p")
        construire_coeur "$v"
        # -DCMAKE_SKIP_RPATH=ON : comme debian/rules — le binaire ne garde aucun
        # chemin de l'arbre de construction.
        # Le cœur de l'étage passe DEVANT les chemins du système, sans les
        # remplacer : sous Nix, gtk+-3.0.pc et ncursesw.pc ne sont trouvés que
        # par le PKG_CONFIG_PATH que pose l'environnement de construction.
        pc="$(etage "$v")/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
        PKG_CONFIG_PATH="$pc" \
            cmake -S "$SRC/sermo-backend-$p" -B "$BUILD/$p" -DCMAKE_SKIP_RPATH=ON $CMAKE_EXTRA
        PKG_CONFIG_PATH="$pc" \
            cmake --build "$BUILD/$p" -j"$JOBS"
    done
    ;;
installer)
    p=${3:-}; dest=${4:-}; prefixe=${5:-/usr}
    [ -n "$p" ] && [ -n "$dest" ] || { echo "port et DESTDIR requis" >&2; exit 2; }
    [ -d "$BUILD/$p" ] || { echo "port non construit : $p ($BUILD/$p)" >&2; exit 1; }
    DESTDIR="$dest" cmake --install "$BUILD/$p" --prefix "$prefixe"
    ;;
coeur-dev)
    dest=${3:-}; prefixe=${4:-/usr}; libdir=${5:-lib}
    [ -n "$dest" ] || { echo "DESTDIR requis" >&2; exit 2; }
    # shellcheck disable=SC2086
    cmake -S "$SRC/libsermocore" -B "$BUILD/coeur-paquet" -DSERMOCORE_LIBDIR="$libdir" \
          -DCMAKE_INSTALL_PREFIX="$prefixe" $CMAKE_EXTRA
    cmake --build "$BUILD/coeur-paquet" -j"$JOBS"
    DESTDIR="$dest" cmake --install "$BUILD/coeur-paquet"
    ;;
manuel)
    dest=${3:-}; prefixe=${4:-/usr}
    [ -n "$dest" ] || { echo "DESTDIR requis" >&2; exit 2; }
    sed "s/@PACKAGE_VERSION@/$VERSION/" "$SRC/doc/sermo.texi.in" > "$BUILD/sermo.texi"
    makeinfo --no-split -o "$BUILD/sermo.info" "$BUILD/sermo.texi"
    install -D -m 0644 "$BUILD/sermo.info" "$dest$prefixe/share/info/sermo-doc.info"
    ;;
*)
    echo "commande inconnue : $cmd" >&2; exit 2 ;;
esac
