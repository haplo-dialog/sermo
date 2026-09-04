#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <button> — Bouton cliquable
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Bouton qt6sermo" width-request="300" height-request="120">
  <vbox>
    <button><label>Cliquez ici !</label><action>echo "Cliqué !"</action></button>
    <button ok></button>
  </vbox>
</window>'
qt6sermo --program MAIN_DIALOG
