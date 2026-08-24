#!/bin/bash
# tests/garde_spdx.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# LICENCES.md affirme que chaque fichier source porte un identifiant SPDX.
# L'affirmation etait fausse (5 fichiers sur 115) ; ce test la rend tenable.
#
# ⚠ grep -a est obligatoire : plusieurs fichiers herites de l'amont sont en
# ISO-8859-1 et grep les prend pour du binaire, donc les SAUTE en silence.
# Sans -a, ce test annoncerait une couverture qu'il n'a pas mesuree — c'est
# exactement l'erreur qui avait fait croire a 30 fichiers sans licence.
SRCDIR="${1:?usage: garde_spdx.sh /chemin/vers/src}"
[[ -d "$SRCDIR" ]] || { echo "repertoire introuvable : $SRCDIR" >&2; exit 2; }

manquants=()
total=0
for f in "$SRCDIR"/*.c "$SRCDIR"/*.h; do
    [[ -e "$f" ]] || continue
    total=$((total+1))
    grep -aq 'SPDX-License-Identifier' "$f" || manquants+=("$(basename "$f")")
done

(( total > 0 )) || { echo "ECHEC : aucun fichier source trouve dans $SRCDIR"; exit 1; }

if (( ${#manquants[@]} > 0 )); then
    echo "ECHEC : ${#manquants[@]} fichier(s) sur $total sans SPDX-License-Identifier :"
    printf '    %s\n' "${manquants[@]}"
    exit 1
fi
echo "OK : $total fichiers source, tous avec un identifiant SPDX"
