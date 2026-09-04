#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <vscale> — Curseur vertical
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
            echo "NIV=${NIV}"
        ;;
    esac
    exit 0
fi

export MAIN_DIALOG='
<window title="VScale qt6sermo" width-request="150" height-request="300">
  <hbox>
    <vscale><variable>NIV</variable><default>30</default></vscale>
    <button ok>
      <action>'"$SOI"' --do afficher</action>
      <action>EXIT:ok</action>
    </button>
  </hbox>
</window>'
"$GTKDIALOG" --program=MAIN_DIALOG
