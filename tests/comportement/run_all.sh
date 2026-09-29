#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# run_all.sh — rejoue le banc de COMPORTEMENT sur les sept backends de rendu et
# rend un tableau récapitulatif. Rend la parité rejouable partout, plus
# seulement sur qt6.
#
# Chaque backend est joué par run.sh (même corpus, étalon gtk3sermo) dans son
# environnement headless. Binaire attendu : sermo-backend-<t>/_build/<t>sermo.
#
# Codes : 0 = les sept backends sont à parité · 1 = au moins un écart, ou un
#         backend absent · 77 = aucun binaire trouvé.
# Un backend non construit est un ÉCHEC, sauf SERMO_PORTS_OPTIONNELS=1 : un
# « tableau 7/7 » qui n'en jouait que cinq restait vert.
set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
RACINE=$(CDPATH= cd -- "$ICI/../.." && pwd)
RUN="$ICI/run.sh"

BACKENDS="gtk3 gtk4 qt6 fltk1 efl1 sdl3 ncurses"
joues=0; kos=0; absents=0
printf '%-10s %s\n' "backend" "résultat"
printf '%-10s %s\n' "-------" "--------"
for t in $BACKENDS; do
    bin="$RACINE/sermo-backend-$t/_build/${t}sermo"
    if [ ! -x "$bin" ]; then
        printf '%-10s %s\n' "$t" "binaire absent (non construit)"
        absents=$((absents + 1)); continue
    fi
    out=$("$RUN" "$bin" 2>&1)
    line=$(printf '%s\n' "$out" | grep -E '^[0-9]+ cas ·' | tail -1)
    if printf '%s\n' "$out" | grep -q 'PARITÉ atteinte'; then
        printf '%-10s ✅ %s\n' "$t" "$line"
        joues=$((joues + 1))
    else
        printf '%-10s ❌ %s\n' "$t" "${line:-échec}"
        printf '%s\n' "$out" | grep -E 'ÉCART|BLOQUÉ|étalon:|rendu :' | sed 's/^/            /'
        kos=$((kos + 1)); joues=$((joues + 1))
    fi
done
echo
printf 'Backends joués : %s · à parité : %s · en écart : %s · absents : %s\n' \
    "$joues" "$((joues - kos))" "$kos" "$absents"
[ "$joues" -eq 0 ] && exit 77
if [ "$absents" -gt 0 ] && [ "${SERMO_PORTS_OPTIONNELS:-0}" != 1 ]; then
    echo "ÉCHEC : $absents backend(s) absent(s) — non joué(s). SERMO_PORTS_OPTIONNELS=1 pour l'accepter."
    exit 1
fi
[ "$kos" -eq 0 ] && exit 0 || exit 1
