#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# run_unit_tests.sh — tests unitaires du cœur, sans affichage.
#
# Compile et exécute :
#  - tests/unit/test_safe_exec.c contre CHAQUE copie de safe_exec.c du cœur :
#    libsermocore/src et libsermocore/src-gtk4. Deux sources, deux épreuves :
#    un correctif qui manquerait à la copie GTK 4 se verrait ici ;
#  - tests/unit/test_sermo_input.c : la limite des <input> (2.7.3), contre
#    libsermocore/src/sermo_input.c ;
#  - tests/unit/test_stringman.c contre libsermocore/src/stringman.c (libcheck).
#
# ⛔ Ne dit jamais OK sans avoir exécuté. La version précédente visait des
# dossiers de la 1.x disparus, sautait tout en silence et affichait « OK ».
# Ici, un outil manquant est une erreur (code 2) et zéro test lancé un échec.
#
# Usage : run_unit_tests.sh
# Codes : 0 = tout passe · 1 = au moins un échec · 2 = outil manquant

set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
RACINE=$(CDPATH= cd -- "$ICI/.." && pwd)
for t in gcc pkg-config; do
    command -v "$t" >/dev/null 2>&1 || { echo "outil manquant : $t" >&2; exit 2; }
done
pkg-config --exists glib-2.0 || { echo "outil manquant : glib-2.0 (libglib2.0-dev)" >&2; exit 2; }

TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
lances=0; echecs=0

lancer() {  # lancer <nom> <binaire>
    printf '──────── %s ────────\n' "$1"
    env -u DISPLAY "$2"; rc=$?
    lances=$((lances + 1))
    if [ "$rc" -eq 0 ]; then printf 'OK     %s\n\n' "$1"; else printf 'ÉCHEC  %s (rc=%s)\n\n' "$1" "$rc"; echecs=$((echecs + 1)); fi
}

for copie in src src-gtk4; do
    dossier="$RACINE/libsermocore/$copie"
    [ -f "$dossier/safe_exec.c" ] || { echo "ÉCHEC : $dossier/safe_exec.c introuvable" >&2; echecs=$((echecs + 1)); continue; }
    bin="$TMP/test_safe_exec_$copie"
    # La copie GTK 4 a son propre safe_exec.h : son dossier passe en premier.
    if gcc -D_GNU_SOURCE $(pkg-config --cflags glib-2.0) -I"$dossier" -I"$RACINE/libsermocore/include" \
           "$ICI/unit/test_safe_exec.c" "$dossier/safe_exec.c" "$RACINE/libsermocore/src/sermo_input.c" \
           $(pkg-config --libs glib-2.0) -o "$bin" 2>"$TMP/cc.log"; then
        lancer "safe_exec ($copie)" "$bin"
    else
        echo "ÉCHEC DE COMPILATION : safe_exec ($copie)"; cat "$TMP/cc.log"; echecs=$((echecs + 1))
    fi
done

bin="$TMP/test_sermo_input"
if gcc -D_GNU_SOURCE $(pkg-config --cflags glib-2.0) -I"$RACINE/libsermocore/include" \
       "$ICI/unit/test_sermo_input.c" "$RACINE/libsermocore/src/sermo_input.c" "$RACINE/libsermocore/src/safe_exec.c" \
       $(pkg-config --libs glib-2.0) -o "$bin" 2>"$TMP/cc.log"; then
    lancer "sermo_input" "$bin"
else
    echo "ÉCHEC DE COMPILATION : sermo_input"; cat "$TMP/cc.log"; echecs=$((echecs + 1))
fi

if pkg-config --exists check gtk+-3.0; then
    bin="$TMP/test_stringman"
    if gcc -D_GNU_SOURCE $(pkg-config --cflags glib-2.0 gtk+-3.0 check) -I"$RACINE/libsermocore/include" \
           "$ICI/unit/test_stringman.c" "$RACINE/libsermocore/src/stringman.c" \
           $(pkg-config --libs glib-2.0 gtk+-3.0 check) -o "$bin" 2>"$TMP/cc.log"; then
        CK_FORK=no lancer "stringman" "$bin"
    else
        echo "ÉCHEC DE COMPILATION : stringman"; cat "$TMP/cc.log"; echecs=$((echecs + 1))
    fi
else
    echo "outil manquant : libcheck ou gtk+-3.0 (check, libgtk-3-dev) — test_stringman NON joué" >&2
    exit 2
fi

if [ "$lances" -eq 0 ]; then
    echo "ÉCHEC : aucun test n'a tourné."; exit 1
fi
echo "Tests unitaires : $lances lancé(s), $echecs échec(s)."
[ "$echecs" -eq 0 ]
