#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <statusbar> — Barre de statut
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
            echo "STATUT=${STATUT}"
        ;;
    esac
    exit 0
fi

export MAIN_DIALOG='
<window title="Status qt6sermo" width-request="400" height-request="150">
  <vbox>
    <text><label>Interface principale</label></text>
    <button><label>Action</label><action>'"$SOI"' --do afficher</action></button>
    <statusbar><variable>STATUT</variable><default>Prêt.</default></statusbar>
    <button ok>
      <action>'"$SOI"' --do afficher</action>
      <action>EXIT:ok</action>
    </button>
  </vbox>
</window>'
"$GTKDIALOG" --program=MAIN_DIALOG
