#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_minuterie_intervalle.sh — haplo-dialog — 2026
#
# Un <timer interval="0"> (ou négatif) ne doit pas tourner sans attente.
# Jusqu'à la 3.3.0, gtk3sermo et gtk4sermo passaient 0 à g_timeout_add_seconds :
# la source se redéclenchait à chaque tour de boucle, un cœur saturé, les
# actions relancées sans arrêt et le reste de la boucle affamé (la seconde
# minuterie du banc ne tiquait plus, le dialogue ne se fermait pas). qt6sermo
# armait un QTimer à 0 ms : 749 tics en 2 s (mesuré).
# L'intervalle est désormais borné à 1 (une seconde, ou une milliseconde avec
# milliseconds="true").
#
# Mesure : une minuterie à interval="0" compte ses tics dans un fichier ; une
# seconde minuterie ferme le dialogue au bout de 2 s. Au plus 4 tics attendus.
#
# Usage : garde_minuterie_intervalle.sh <binaire d'un port>
# Codes : 0 = tient · 1 = défaut · 2 = outil ou binaire manquant
set -u
BIN="${1:?usage: garde_minuterie_intervalle.sh /chemin/vers/binaire}"
[ -x "$BIN" ] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
command -v xvfb-run >/dev/null || { echo "outil manquant : xvfb-run" >&2; exit 2; }
BIN=$(CDPATH= cd -- "$(dirname -- "$BIN")" && pwd)/$(basename -- "$BIN")
T=$(mktemp -d) || exit 2; trap 'rm -rf "$T"' EXIT
cd "$T" || exit 2
export SERMO_NCURSES_BATCH=1 SERMO_HTML_OUT=/dev/null
export MAIN_DIALOG='<window><vbox><text><label>minuterie</label></text>
<timer interval="0" visible="false"><variable>_Z</variable><action>sh -c "echo t >> tics"</action></timer>
<timer interval="2" visible="false"><variable>_F</variable><action>exit:fin</action></timer>
</vbox></window>'
GSK_RENDERER=cairo GTK_A11Y=none xvfb-run -a timeout 25 "$BIN" --program=MAIN_DIALOG >sortie 2>err
grep -q '^EXIT="fin"' sortie || { echo "ECHEC : le dialogue ne s'est pas fermé par sa minuterie"; exit 1; }
tics=$(wc -l < tics 2>/dev/null || echo 0)
[ "$tics" -ge 1 ] || { echo "ECHEC : la minuterie à interval=\"0\" n'a jamais tiqué"; exit 1; }
[ "$tics" -le 4 ] || { echo "ECHEC : $tics tics en 2 s pour interval=\"0\" (4 au plus attendus)"; exit 1; }
echo "OK : <timer interval=\"0\"> : $tics tic(s) en 2 s, pas de boucle sans attente"
