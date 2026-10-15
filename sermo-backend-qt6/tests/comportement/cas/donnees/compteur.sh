#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Compteur à résultat fixe (cas 55, 56 et 73) : chaque appel lit ./cpt (0 s'il
# manque), ajoute 1, réécrit, affiche. Le premier appel est celui de l'<input>
# à l'ouverture ; le second ne vient que si l'action « refresh » le rejoue.
# Un port qui ignore « refresh » rend donc 1, et le cas rougit.
# Argument facultatif : le fichier du compteur (un par widget, cas 75 et suivants).
f=${1:-cpt}
n=$( [ -f "$f" ] && cat "$f" 2>/dev/null || echo 0 )
n=$((n + 1))
echo "$n" > "$f"
printf '%s\n' "$n"
