#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Pour un bouton radio (cas 76) : compte ses appels dans le fichier donné
# (./cpt par défaut) ; « false » au premier, « true » ensuite.
f=${1:-cpt}
n=$( [ -f "$f" ] && cat "$f" 2>/dev/null || echo 0 )
n=$((n + 1))
echo "$n" > "$f"
if [ "$n" -eq 1 ]; then echo false; else echo true; fi
