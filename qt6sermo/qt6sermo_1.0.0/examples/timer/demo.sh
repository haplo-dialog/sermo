#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <timer> — Déclencheur temporisé
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Timer qt6sermo" width-request="350" height-request="150">
  <vbox>
    <text><label>Ce dialogue se ferme dans 5 secondes.</label></text>
    <timer><variable>TICK</variable><value>5000</value><action>EXIT:0</action></timer>
    <button><label>Fermer</label><action>EXIT:0</action></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "TICK=${TICK}"
