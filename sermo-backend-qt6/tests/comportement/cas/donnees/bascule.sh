#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Pour les widgets à bascule (cas 76) : compte ses appels dans le fichier donné
# (./cpt par défaut) et affiche « true » aux appels impairs, « false » aux
# pairs. Le premier appel est celui de l'<input> juste après la création ; le
# second ne vient que si « refresh » le rejoue.
f=${1:-cpt}
n=$( [ -f "$f" ] && cat "$f" 2>/dev/null || echo 0 )
n=$((n + 1))
echo "$n" > "$f"
if [ $((n % 2)) -eq 1 ]; then echo true; else echo false; fi
