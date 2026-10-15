#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_poids.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# E08 : le poids annoncé est le poids réel. Mesure chaque binaire après
# « strip --strip-unneeded » (comme dans le paquet), l'affiche, et le compare
# au plafond de son port (tests/poids-plafonds.txt). Un dépassement est un
# échec : on allège, ou on relève le plafond en l'écrivant (choix assumé).
#
# Usage : garde_poids.sh [racine] [port…]
# Codes : 0 = sous les plafonds · 1 = un dépassement · 2 = binaire ou outil absent
set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" && pwd) || exit 2
shift 2>/dev/null
PORTS=("$@"); [ ${#PORTS[@]} -gt 0 ] || PORTS=(gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl ncurses html)
command -v strip >/dev/null || { echo "outil manquant : strip (binutils)" >&2; exit 2; }
T=$(mktemp -d -p "$RACINE/_build" poids.XXXXXX 2>/dev/null) || { mkdir -p "$RACINE/_build" && T=$(mktemp -d -p "$RACINE/_build" poids.XXXXXX); } || exit 2
trap 'rm -rf -- "$T"' EXIT
echecs=0
printf '%-8s %8s %8s\n' port Kio plafond
for p in "${PORTS[@]}"; do
    case "$p" in
        lvgl-leger) b="$RACINE/sermo-backend-lvgl/_build_leger/lvglsermo" ;;
        *)          b="$RACINE/sermo-backend-$p/_build/${p}sermo" ;;
    esac
    [ -x "$b" ] || { echo "binaire absent : $b" >&2; exit 2; }
    plafond=$(awk -v p="$p" '$1 == p { print $3 }' "$RACINE/tests/poids-plafonds.txt")
    [ -n "$plafond" ] || { echo "ÉCHEC : aucun plafond pour $p"; echecs=$((echecs + 1)); continue; }
    cp -- "$b" "$T/b" && strip --strip-unneeded "$T/b" || exit 2
    kio=$(( $(stat -c %s "$T/b") / 1024 ))
    printf '%-8s %8d %8d\n' "$p" "$kio" "$plafond"
    [ "$kio" -le "$plafond" ] || { echo "ÉCHEC : $p pèse $kio Kio, plafond $plafond"; echecs=$((echecs + 1)); }
done
[ "$echecs" = 0 ] && echo "OK : poids sous les plafonds (${#PORTS[@]} ports)"
[ "$echecs" = 0 ]
