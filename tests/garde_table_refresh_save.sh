#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_table_refresh_save.sh — haplo-dialog — 2026
#
# refresh: et save: d'une <table>. L'étalon gtk3sermo AJOUTE au refresh les
# lignes de <input> (2 au départ, 4 après), et save: écrit une rangée par
# ligne, cellules séparées par « | », sans saut de ligne final. Jusqu'à la
# 3.2.0, gtk4sermo ne faisait ni l'un ni l'autre (fonctions vides).
#
# Ports GTK seulement : save: n'écrit rien sur les sept autres (écart connu).
# Usage : garde_table_refresh_save.sh <gtk3sermo|gtk4sermo>
# Codes : 0 = tient · 1 = défaut · 2 = outil ou binaire manquant
set -u
BIN="${1:?usage: garde_table_refresh_save.sh /chemin/vers/binaire}"
[ -x "$BIN" ] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
command -v xvfb-run >/dev/null || { echo "outil manquant : xvfb-run" >&2; exit 2; }
BIN=$(CDPATH= cd -- "$(dirname -- "$BIN")" && pwd)/$(basename -- "$BIN")
T=$(mktemp -d) || exit 2; trap 'rm -rf "$T"' EXIT
cd "$T" || exit 2
export MAIN_DIALOG='<window><vbox><table><variable>TB</variable><label>a|b</label><input>printf "1|2\n3|4\n"</input><output file>sortie-table.txt</output></table><timer visible="false"><variable>T</variable><action>refresh:TB</action><action>save:TB</action><action>exit:fin</action></timer></vbox></window>'
GSK_RENDERER=cairo GTK_A11Y=none xvfb-run -a timeout 25 "$BIN" --program=MAIN_DIALOG >sortie 2>err
attendu=$(printf '1|2\n3|4\n1|2\n3|4')
[ -f sortie-table.txt ] || { echo "ECHEC : save: n'a écrit aucun fichier"; exit 1; }
[ "$(cat sortie-table.txt)" = "$attendu" ] && [ "$(tail -c 1 sortie-table.txt)" = 4 ] \
    || { echo "ECHEC : fichier écrit [$(tr '\n' '/' <sortie-table.txt)] ; « 1|2/3|4/1|2/3|4 » attendu, sans saut de ligne final"; exit 1; }
echo "OK : <table> : refresh: ajoute les lignes de <input>, save: écrit les quatre rangées"
