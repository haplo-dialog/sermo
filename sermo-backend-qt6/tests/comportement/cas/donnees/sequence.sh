#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# sequence.sh <fichier> <valeur 1> <valeur 2> … (cas 78) : compte ses appels
# dans le fichier donné et affiche la valeur de rang égal au nombre d'appels
# (la dernière une fois la liste épuisée). Le premier appel est celui de
# l'<input> juste après la création ; les suivants viennent de « refresh ».
f=$1
shift
n=$( [ -f "$f" ] && cat "$f" 2>/dev/null || echo 0 )
n=$((n + 1))
echo "$n" > "$f"
i=1
for v in "$@"; do
    dernier=$v
    if [ "$i" -eq "$n" ]; then printf '%s\n' "$v"; exit 0; fi
    i=$((i + 1))
done
printf '%s\n' "$dernier"
