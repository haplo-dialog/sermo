#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <hbox> — Conteneur horizontal
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="HBox qt6sermo" width-request="400" height-request="120">
  <hbox>
    <button><label>Gauche</label></button>
    <button><label>Centre</label></button>
    <button><label>Droite</label></button>
  </hbox>
</window>'
qt6sermo --program MAIN_DIALOG
