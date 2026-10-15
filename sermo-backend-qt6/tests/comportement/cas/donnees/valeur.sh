#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# valeur.sh <genre> [fichier] (cas 77) : compte ses appels dans le fichier donné
# (./cpt par défaut) et affiche la n-ième valeur du genre demandé. Le premier
# appel est celui de l'<input> juste après la création ; les suivants ne
# viennent que si « refresh » le rejoue.
g=$1
f=${2:-cpt}
n=$( [ -f "$f" ] && cat "$f" 2>/dev/null || echo 0 )
n=$((n + 1))
echo "$n" > "$f"
case $g in
  date)    echo "2021-0$n-0$n" ;;
  couleur) echo "#00000$n" ;;
  police)  echo "Sans 1$n" ;;
  texte)   echo "v$n" ;;
  uri)     echo "https://exemple.invalid/$n" ;;
  bascule) if [ $((n % 2)) -eq 1 ]; then echo true; else echo false; fi ;;
  page)    echo $((n - 1)) ;;
esac
