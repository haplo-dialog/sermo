#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <window> — Fenêtre principale avec barre de menus optionnelle
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Fenêtre qt6sermo" width-request="400" height-request="200">
  <vbox>
    <text><label>Bienvenue dans qt6sermo !</label></text>
    <button><label>Fermer</label><action>EXIT:0</action></button>
  </vbox>
</window>'
qt6sermo --program MAIN_DIALOG
