#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <timer> — Déclencheur temporisé
# Port : qt6sermo (Qt6 >= 6.2.0)
# Motif « --do » : le dialogue rappelle CE script pour agir — les valeurs des
# widgets lui arrivent par l'environnement, jamais par une ligne rejouée au
# shell. (L'ancien « eval $(...) » évaluait la sortie du dialogue : c'est le
# geste que la famille sermo déconseille.)

GTKDIALOG=${GTKDIALOG:-qt6sermo}
SOI=$(readlink -f "$0")

if [ "${1-}" = --do ]; then
    case "${2-}" in
        afficher)
            echo "TICK=${TICK}"
        ;;
    esac
    exit 0
fi

export MAIN_DIALOG='
<window title="Timer qt6sermo" width-request="350" height-request="150">
  <vbox>
    <text><label>Ce dialogue se ferme dans 5 secondes.</label></text>
    <timer interval="5"><variable>TICK</variable><action>'"$SOI"' --do afficher</action><action>EXIT:0</action></timer>
    <button><label>Fermer</label><action>'"$SOI"' --do afficher</action><action>EXIT:0</action></button>
  </vbox>
</window>'
"$GTKDIALOG" --program=MAIN_DIALOG
