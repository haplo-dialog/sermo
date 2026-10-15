#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_sanitizers.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# E07 : aucun défaut mémoire ni comportement indéfini connu. Construit le cœur
# (trois variantes) et les ports donnés sous ASan + UBSan, dans des dossiers à
# part (_build_asan*, rien d'écrasé), puis joue le banc de comportement sur
# chacun, avec toute erreur FATALE : un rapport arrête le cas, le banc le voit.
#
# Pourquoi fatal : en build combiné, UBSan n'écrit que sur la sortie d'erreur
# (que le banc de comportement jette) et ASan écrit dans le log_path
# d'UBSAN_OPTIONS ; compter des fichiers de rapport donnait « 0 » à tort
# (mesuré le 14/10). TÉMOIN : un petit programme fautif, construit avec les
# mêmes options, DOIT s'arrêter ; sinon rien n'est vérifié (code 77).
# Les fuites ne sont pas jugées ici (les toolkits en laissent qui ne sont pas
# les nôtres).
#
# Usage : garde_sanitizers.sh [racine] [port…]   (~40 min pour les neuf)
# Codes : 0 = tout tient · 1 = un défaut · 2 = outil absent · 77 = témoin muet
set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" && pwd) || exit 2
shift 2>/dev/null
PORTS=("$@"); [ ${#PORTS[@]} -gt 0 ] || PORTS=(gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl ncurses html)
SAN="-fsanitize=address,undefined -fno-omit-frame-pointer -g -O1"
export ASAN_OPTIONS="detect_leaks=0:abort_on_error=1"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:abort_on_error=1"
N=$(nproc 2>/dev/null || echo 2)
J="$RACINE/_build/sanitizers"; mkdir -p "$J" || exit 2
unset PKG_CONFIG_PATH DISPLAY WAYLAND_DISPLAY

# Témoin : un débordement d'entier et une écriture hors tampon (volatile : le
# compilateur ne peut pas les retirer).
printf '#include <stdlib.h>\n#include <limits.h>\nint main(int c,char**v){(void)v;volatile char*p=malloc(8);int x=INT_MAX;x+=c;p[8]=(char)x;free((void*)p);return 0;}\n' > "$J/temoin.c"
cc $SAN -o "$J/temoin" "$J/temoin.c" 2>"$J/temoin-cc.log" || { echo "outil absent : compilateur avec -fsanitize"; exit 2; }
{ "$J/temoin"; } >/dev/null 2>&1 && { echo "IGNORÉ : le témoin fautif ne s'arrête pas — rien n'est vérifié"; exit 77; }

echecs=0
for v in neutre glib gtk4; do
    case $v in glib) o=-DSERMOCORE_GLIB=ON ;; gtk4) o=-DSERMOCORE_GTK4=ON ;; *) o= ;; esac
    { cmake -S "$RACINE/libsermocore" -B "$RACINE/libsermocore/_build_asan_$v" $o \
        -DCMAKE_INSTALL_PREFIX="$RACINE/libsermocore/_install_asan_$v" -DCMAKE_C_FLAGS="$SAN" -DCMAKE_CXX_FLAGS="$SAN" \
      && cmake --build "$RACINE/libsermocore/_build_asan_$v" -j"$N" && cmake --install "$RACINE/libsermocore/_build_asan_$v"; } \
      > "$J/coeur-$v.log" 2>&1 || { echo "ÉCHEC : cœur $v sous sanitizers (voir $J/coeur-$v.log)"; echecs=$((echecs + 1)); }
done
for p in "${PORTS[@]}"; do
    case $p in gtk3) v=glib ;; gtk4) v=gtk4 ;; *) v=neutre ;; esac
    b="$RACINE/sermo-backend-$p/_build_asan/${p}sermo"
    if ! { PKG_CONFIG_PATH="$RACINE/libsermocore/_install_asan_$v/lib/pkgconfig" cmake -S "$RACINE/sermo-backend-$p" -B "$RACINE/sermo-backend-$p/_build_asan" \
             -DCMAKE_C_FLAGS="$SAN" -DCMAKE_CXX_FLAGS="$SAN" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
           && PKG_CONFIG_PATH="$RACINE/libsermocore/_install_asan_$v/lib/pkgconfig" cmake --build "$RACINE/sermo-backend-$p/_build_asan" -j"$N"; } \
         > "$J/port-$p.log" 2>&1 || ! nm "$b" 2>/dev/null | grep -q __asan_init; then
        echo "ÉCHEC : $p non construit sous sanitizers (voir $J/port-$p.log)"; echecs=$((echecs + 1)); continue
    fi
    if TIMEOUT=60 sh "$RACINE/tests/comportement/run.sh" "$b" > "$J/comportement-$p.log" 2>&1; then
        echo "OK     $p : $(tail -1 "$J/comportement-$p.log" | cut -c1-60)"
    else
        echo "ÉCHEC : $p sous ASan/UBSan fatals — $(tail -1 "$J/comportement-$p.log")"; echecs=$((echecs + 1))
    fi
done
[ "$echecs" = 0 ] && echo "OK : ${#PORTS[@]} port(s) sans erreur ASan ni UBSan (témoin rouge vu)"
[ "$echecs" = 0 ]
